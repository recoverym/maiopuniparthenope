<?php
/**
 * @file
 * API functions for the ls_survey module
 */

/**
 * Extract surveys with their properties from the LimeSurvey database.
 *
 * This function extracts surveys with their properties from the LimeSurvey
 * and returns them as an php array variable. It extracts all survey or only
 * a specific survey, depending on the parameter $sid.
 *
 * @param $ls_sid
 *   An integer, corresponding to the survey reference ID or 'all' to return
 *   to all surveys stored on the LimeSurvey database keyed by the survey ID.
 * @param $reset
 *   A boolean, set it to TRUE to reload cached datas. Defalt is FALSE.
 *
 * @return
 *   An multidimensionnal array, corresponding to surveys with their
 *   properties from the LimeSurvey database.
 */
function limesurvey_sync_survey_properties($ls_sid = NULL, $reset = FALSE) {
  // Cache datas.
  static $all_surveys = array();
  if ($reset || empty($all_surveys)) {
    $all_surveys = array();

    // Load file to connect to the Limesurvey tables.
    module_load_include('inc', 'limesurvey_sync', 'ls_api/limesurvey_sync_api_sync');
    // Load file for the drupal formated url.
    module_load_include('inc', 'limesurvey_sync', 'limesurvey_sync');
    // Load file for the ls_answ_clean_ls_date(() function.
    module_load_include('inc', 'ls_answ', 'ls_answ');

    // Load all existing drupal surveys.
    $surveys_query = db_query("SELECT n.nid AS node_nid, ls_sid, ls_lang, nt.type AS answ_type
    FROM {node} n
    JOIN {ls_survey} lss ON lss.nid = n.nid AND lss.vid = n.vid
    JOIN {node_type} nt ON lss.ls_orig_type = nt.orig_type");
    // Don't restrict to module = ls_answ : the assocation will still works if
    // the ls_answ module is disabled.
    while ($surveys_row = $surveys_query->fetchAssoc()) {
      $nodes_survey[$surveys_row['ls_sid']][$surveys_row['ls_lang']]['nid_survey'] = $surveys_row['node_nid'];
      $nodes_survey[$surveys_row['ls_sid']][$surveys_row['ls_lang']]['answ_type'] = $surveys_row['answ_type'];
    }

    // Retrieve the database version.
    limesurvey_sync_db_open();
    $db_version = @db_query('SELECT stg_value FROM {' . limesurvey_sync_table('settings_global') . '} WHERE stg_name = :stg_value', array(':stg_value' => 'DBVersion'))->fetchField();
    if ($db_version === FALSE) {
      limesurvey_sync_db_close();
      return FALSE;
    }

    // Load all surveys properties from the LimeSurvey database.
    $desquery = db_query('SELECT surveys.*, surveys_lang.* FROM {' . limesurvey_sync_table('surveys') . '} surveys '
    . 'RIGHT JOIN {' . limesurvey_sync_table('surveys_languagesettings') . '} surveys_lang ON surveys_lang.surveyls_survey_id = surveys.sid '
    . 'ORDER BY active DESC, sid ASC, surveyls_language ASC, surveyls_title ASC');
    // Iterate each survey.
    foreach ($desquery as $surveys) {
      $sid = check_plain($surveys->sid);
      $lang = check_plain($surveys->surveyls_language);

      // Save datas into a php array variable.

      // Populate the non language specific properties.
      if (!isset($all_surveys[$sid])) {
        $all_surveys[$sid] = array(
          'active' => (check_plain($surveys->active) == 'Y'),
          'tokens_table' => db_table_exists(limesurvey_sync_table('tokens', $sid)),
          'tokenanswerspersistence' => (check_plain($surveys->tokenanswerspersistence) == 'Y'),
          // For old versions of LS (dbversion < 145) : set to TRUE if it not
          // exists, to allow to edit answers.
          'alloweditaftercompletion' => (isset($surveys->alloweditaftercompletion)) ? (check_plain($surveys->alloweditaftercompletion) == 'Y') : TRUE,
          // The showwelcome has been added on dbversion = 145 to.
          'showwelcome' => (isset($surveys->showwelcome)) ? (check_plain($surveys->showwelcome) == 'Y') : TRUE,
          // The 'tokenlength' colomn has been added since LimeSurvey DBversion >= 141.
          'tokenlength' => (isset($surveys->tokenlength)) ? check_plain($surveys->tokenlength) : 15,
          'autoredirect' => (check_plain($surveys->autoredirect) == 'Y'),
          'default_language' => check_plain($surveys->language),
          'additional_languages' => check_plain(trim($surveys->additional_languages)),
          'anonymized' => (check_plain($surveys->anonymized) == 'Y'),
          'date_answer' => (check_plain($surveys->datestamp) == 'Y'),
          'ipaddr' => (check_plain($surveys->ipaddr) == 'Y'),
          'refurl' => (check_plain($surveys->refurl) == 'Y'),
          // 'publicstatistics' colomn has been added since LimeSurvey
          // DBversion >= 131.
          'publicstatistics' => (isset($surveys->publicstatistics)) ? (check_plain($surveys->publicstatistics) == 'Y') : FALSE,
          // Note : bd version is the same for all surveys.
          'db_version' => check_plain($db_version),
          // 'startdate' colomn has been added since LimeSurvey DBversion >= 129.
          // 'startdate' and 'expires' are NULL by default. Change them to string.
          'startdate' => (empty($surveys->startdate)) ? '' : check_plain($surveys->startdate),
          'expires' => (empty($surveys->expires)) ? '' : check_plain($surveys->expires),
        );

        // Here add to use token persistence only if token table exist.
        // $all_surveys[$sid]['compatibility'] = (!$all_surveys[$sid]['tokenanswerspersistence'] || $all_surveys[$sid]['tokens_table']);


        // Check for disabled survey.
        $time = REQUEST_TIME;
        $survey_has_started = (empty($all_surveys[$sid]['startdate']) || $time >= ls_answ_clean_ls_date($all_surveys[$sid]['startdate']));
        $survey_not_finished = (empty($all_surveys[$sid]['expires']) || $time < ls_answ_clean_ls_date($all_surveys[$sid]['expires']));
        $survey_is_actived = $all_surveys[$sid]['active'];
        $is_available = ($survey_has_started && $survey_not_finished && $survey_is_actived);
        $all_surveys[$sid]['is_available'] = $is_available;

        $all_surveys[$sid]['can_edit_answer'] = ($all_surveys[$sid]['tokens_table'] && $all_surveys[$sid]['tokenanswerspersistence'] && $all_surveys[$sid]['alloweditaftercompletion']);

        // Check if http_referer method is available.
        $http_referer = FALSE;
        if ($all_surveys[$sid]['autoredirect'] && !$all_surveys[$sid]['showwelcome']) {
          // Check if it is a one page survey.
          switch ($surveys->format) {
            case 'A': // All in one.
              $http_referer = TRUE;
              break;
            case 'G': // Group by group.
              // Check if there is only one group.
              $query_group = db_query('SELECT COUNT(*) FROM {' . limesurvey_sync_table('groups') . '} WHERE sid = :sid GROUP BY gid', array(':sid' => $sid));
              $http_referer = ($query_group->fetchField() <= 1);
              break;
            case 'S': // Question by question
              // Check if there is only one question.
              $query_question = db_query('SELECT COUNT(*) FROM {' . limesurvey_sync_table('questions') . '} WHERE sid = :sid GROUP BY qid', array(':sid' => $sid));
              $http_referer = ($query_question->fetchField() <= 1);
              break;
          }
        }
        $all_surveys[$sid]['http_referer'] = $http_referer;
      }

      // Populate the language specific properties.

      // Handle back url from LS, comparing to the drupal url() version.
      $ls_settings = variable_get('ls_settings', array('ls_path' => ''));
      $url_lang = check_plain($surveys->surveyls_url);
      $back_url_survey = FALSE;
      if (!empty($url_lang)) {
        // Convert relative to absolute url if needed and format the url into the drupal way.
        $url_lang = limesurvey_sync_rel2abs($url_lang, limesurvey_sync_rel2abs($ls_settings['ls_path'] . '/'));
        $back_url_survey = limesurvey_sync_drupal_formated_url($url_lang);
      }
      $correct_back_url = $type = FALSE;
      if (!empty($nodes_survey[$sid][$lang]['answ_type'])) {
        $type = $nodes_survey[$sid][$lang]['answ_type'];
        $path = 'backfromsurvey/' . $type;
        $options = variable_get('ls_answ_options_' . $type, FALSE);
        $iframes = array('iframe', 'lightbox', 'modaliframe');
        if ($options && in_array($options['ls_redirect'], $iframes)) {
          $path .= '/back';
        }
        $correct_back_url = url($path, array('absolute' => TRUE));
      }

      // Populate the language specific properties.
      $all_surveys[$sid]['#lang'][$lang] = array(
        'title' => check_plain($surveys->surveyls_title),
        'description' => filter_xss($surveys->surveyls_description),
        'back_url' => check_plain($surveys->surveyls_url),
        'back_to_drupal' => ($back_url_survey && $back_url_survey == $correct_back_url),
        'correct_back_url' => $correct_back_url,
        // Note that, in most cases, nid_survey and type should not be retreive
        // from this function because it does not check if the ls_answ_module
        // is enabled and uses the current available value, not the node one
        // (which may be not the same on desynchronized answers). It is used
        // only for not breaking informations in case of the ls_answ module
        // would not be enabled.
        'nid_survey' => (!empty($nodes_survey[$sid][$lang]['nid_survey'])) ? $nodes_survey[$sid][$lang]['nid_survey'] : FALSE,
        'type' => $type,
      );
    }
    limesurvey_sync_db_close();
  }

  // Deal with request :
  if ($ls_sid == 'all') {
    return $all_surveys;
  }
  elseif (isset($all_surveys[$ls_sid])) {
    return $all_surveys[$ls_sid];
  }
  else {
    return FALSE;
  }
  //
  // Example of the returned datas :
  // $all_surveys = Array
  // (
  //   [75158] => Array
  //   (
  //     [active] => TRUE
  //     [tokens_table] => TRUE
  //     [tokenanswerspersistence] => TRUE
  //     [alloweditaftercompletion] => TRUE
  //     [showwelcome] => TRUE
  //     [tokenlength] => 15
  //     [autoredirect] => TRUE
  //     [default_language] => fr
  //     [additional_languages] =>
  //     [anonymized] => TRUE
  //     [date_answer] => TRUE
  //     [ipaddr] => FALSE
  //     [refurl] => FALSE
  //     [publicstatistics] => TRUE
  //     [db_version] => TRUE
  //     [startdate] => ''
  //     [expires] => ''
  //     [is_available] => TRUE
  //     [can_edit_answer] => TRUE
  //     [http_referer] => TRUE
  //     [#lang] => Array
  //     (
  //       [fr] => Array
  //       (
  //         [title] => survey1
  //         [description] => this survey do this
  //         [back_url] => ./../site/?q=backfromsurvey/answ_type1
  //         [back_to_drupal] => TRUE
  //         [correct_back_url] => http://www.example.com/site/?q=backfromsurvey/answ_type1
  //         [nid_survey] => 12
  //         [type] => answ_type1
  //       )
  //
  //     )
  //
  //  )
  //
  //
  //  [39326] => Array
  //  (
  //     [active] => TRUE
  //     [tokens_table] => TRUE
  //     [tokenanswerspersistence] => TRUE
  //     [alloweditaftercompletion] => TRUE
  //     [showwelcome] => TRUE
  //     [tokenlength] => 15
  //     [autoredirect] => TRUE
  //     [default_language] => fr
  //     [additional_languages] =>
  //     [anonymized] => TRUE
  //     [date_answer] => TRUE
  //     [ipaddr] => FALSE
  //     [refurl] => FALSE
  //     [publicstatistics] => TRUE
  //     [db_version] => TRUE
  //     [startdate] => ''
  //     [expires] => ''
  //     [is_available] => TRUE
  //     [can_edit_answer] => TRUE
  //     [http_referer] => TRUE
  //     [#lang] => Array
  //     (
  //       [fr] => Array
  //       (
  //         [title] => survey3 (fr)
  //         [description] => blablabla
  //         [back_url] => ./../site/?q=backfromsurvey/answ_type2
  //         [back_to_drupal] => TRUE
  //         [correct_back_url] => http://www.example.com/site/?q=backfromsurvey/answ_type2
  //         [nid_survey] => 5
  //         [type] => answ_type2
  //       )
  //
  //       [es] => Array
  //       (
  //         [title] => survey3 (es)
  //         [description] => blablabla
  //         [back_url] => ./../site/
  //         [back_to_drupal] => FALSE
  //         [correct_back_url] => http://www.example.com/site/?q=backfromsurvey/answ_type3
  //         [nid_survey] => 6
  //         [type] => answ_type3
  //       )
  //
  //       [en] => Array
  //       (
  //         [title] => survey3 (en)
  //         [description] => blablabla
  //         [back_url] =>
  //         [back_to_drupal] => FALSE
  //         [correct_back_url] => http://www.example.com/site/?q=backfromsurvey/answ_type1
  //         [nid_survey] => 7
  //         [type] => answ_type4
  //       )
  //
  //     )
  //   )
  //
  // )
}
