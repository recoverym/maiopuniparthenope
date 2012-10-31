<?php
/**
 * @file
 * API functions for the ls_sync module
 */

/**
 * Activate the LimeSurvey database.
 *
 * Function called before each query or group of querys on the Limesurvey
 * database.
 */
function limesurvey_sync_db_open() {
  db_set_active('ls');
}

/**
 * Desactivate the LimeSurvey database.
 *
 * Function called after each query or group of querys on the Limesurvey
 * database, to allow the system to switch back to the Drupal database.
 */
function limesurvey_sync_db_close() {
  db_set_active('default');
}

/**
 * Provide the LimeSurvey database table name.
 *
 * Depending on the selected table, concatenate the LimeSurvey database table
 * name to the survey ID. For example,
 * limesurvey_sync_table('surveys', 12356) returns 'surveys_123456'.
 *
 * @param $table
 *   A string. Corresponding to the LimeSurvey table generic name. Default is
 *   'survey'.
 * @param $sid
 *   (optional) An integer. Corresponding to the survey ID reference. Default
 *   is empty.
 *
 * @return
 *   A string, corresponding to the LimeSurvey database name.
 */
function limesurvey_sync_table($table = 'surveys', $sid = '') {
  switch ($table) {
    // Handle tokens and answers tables : they need the $sid parameter.
    case 'tokens':
    case 'survey':
    case 'answer':
      // Watch out ! The answerS table exists to in the Limesurvey schema !!
      if ($table == 'answer') {
        // Answer is an more explicit name for the survey answers table name.
        $table = 'survey';
      }
      $table_name = $table . '_' . (string) $sid;
      break;
    default:
      $table_name = $table;
  }
  return check_plain($table_name);
}

/**
 * Provide url to specific pages of the LimeSurvey admin interface.
 *
 * @param $sid
 *   An numeric. Corresponding to the survey reference ID. Default is NULL.
 * @param $page
 *   A string. The page. Check the function for available values. Default is
 *   'admin'.
 * @param $anchor
 *   An string. The anchor value. Default is empty.
 * @param $absolute
 *   A boolean. For absolute url. Default is TRUE.
 *
 * @return
 *   A string, The corresponding url.
 */
function limesurvey_sync_admin_url($sid = NULL, $page = 'admin', $anchor = '', $absolute = TRUE) {
  $options = array(
    'absolute' => $absolute,
    'external' => TRUE,
  );

  $ls_settings = variable_get('ls_settings', array('ls_path' => ''));
  $url = $ls_settings['ls_path'] . '/admin/admin.php';
  if ($absolute) {
    module_load_include('inc', 'limesurvey_sync', 'limesurvey_sync');
    $url = limesurvey_sync_rel2abs($url);
  }

  switch ($page) {
    case 'editsurveysettings':
      $options['query']['action'] = 'editsurveysettings';
      break;
    case 'newsurvey':
      $options['query']['action'] = 'newsurvey';
      break;
    case 'activate':
      $options['query']['action'] = 'activate';
      break;
    case 'statistics':
      $options['query']['action'] = 'statistics';
      break;
    case 'tokens':
      $options['query']['action'] = 'tokens';
      break;
    case 'browse':
      $options['query']['action'] = 'browse';
      break;
    case 'listsurveys':
      $options['query']['action'] = 'listsurveys';
      break;
    case 'editsurveylocalesettings':
      $options['query']['action'] = 'editsurveylocalesettings';
      break;
    case 'globalsettings':
      $options['query']['action'] = 'globalsettings';
      break;
  }

  switch ($anchor) {
    case 'presentation':
      $options['fragment'] = 'presentation';
      break;
    case 'tokens':
      $options['fragment'] = 'tokens';
      break;
    case 'general':
      $options['fragment'] = 'general';
      break;
    case 'publication':
      $options['fragment'] = 'publication';
      break;
    case 'notification':
      $options['fragment'] = 'notification';
      break;
    case 'copy':
      $options['fragment'] = 'copy';
      break;
    case 'edittxtele0':
      // @TODO : 'edittxtele0' stands for default language, 'edittxtele1' for
      // the first additional language, etc...
      $options['fragment'] = 'edittxtele0';
      break;
  }

  switch ($page) {
    case 'admin':
    case 'editsurveysettings':
    case 'activate':
    case 'statistics':
    case 'tokens':
    case 'browse':
    case 'editsurveylocalesettings':
      if (!empty($sid)) {
        $options['query']['sid'] =  $sid;
      }
      else {
        // If sid id not provide, those pages do not exist.
        // Go to the defaut admin page instead.
        unset($options['query']['action'], $options['fragment']);
      }
      break;
    default:
  }

  // Set 'sid' before 'action' in the url.
  if (isset($options['query']) && is_array($options['query'])) {
    krsort($options['query']);
  }

  return url($url, $options);
}
