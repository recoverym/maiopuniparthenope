<?php
/**
 * @file
 * fonctions for the answers content type edit page.
 */

/**
 * Provide default values for the answer node type options.
 *
 * @return
 *   An array of the each option default value.
 */
function ls_survey_answ_default_options() {
  $time = REQUEST_TIME;
  return array(
    'ls_prefix' => 'Response #',
    'ls_suffix_method' => 'numeric',
    'ls_suffix_min' => 1000,
    'ls_suffix_max' => 9999,
    'ls_disabled_title' => 0,
    'ls_redirect' => 'redirect',
    'ls_validuntil' => 7200,
    'ls_end' => 'page',
    'ls_end_url' => '',
    'ls_only_answered' => 0,
    'ls_merge' => 1,
    'ls_ignore_x' => 0,
    'ls_fields' => array(),
    'ls_auto_import' => 0,
    'ls_author' => '',
    'ls_standing_by_status' => 0,
    'ls_export_token' => 0,
    'ls_keep_revisions' => 0,
    'ls_revision' => 0,
    'ls_recycling' => 0,
    'ls_format' => 'full_html',
    'ls_options_last_import' => $time,
  );
}

/**
 * Display the answers content type edit page.
 *
 * Display the answers content type edit page on the options tab of the
 * associated survey node.
 */
function ls_survey_type($node) {
  module_load_include('inc', 'ls_survey', 'ls_survey');
  $type = ls_survey_find_answ_type($node->ls_orig_type);
  return drupal_get_form('node_type_form', $type);
}

/**
 * Form builder for the answers content type options.
 *
 * Set options answers content type on a specific fieldset
 *
 * @see ls_survey_answ_options_validate()
 * @see ls_survey_node_type_validate()
 * @see ls_survey_answ_options_submit()
 * @see ls_survey_answ_options_redirect()
 * @ingroup forms
 */
function ls_survey_answ_options($form) {
  $node_type = $form['#node_type'];

  // Load appropriate variable, depending on the type
  $variable_name = ($node_type->module == 'ls_survey') ? 'ls_survey_options' : 'ls_answ_options_' . $node_type->type;
  $answer_options = variable_get($variable_name, FALSE);
  if (!$answer_options) {
    $answer_options = variable_get('ls_survey_options', ls_survey_answ_default_options());
  }

  $sid = $answ_type = FALSE;
  if ($node_type->module != 'ls_survey' && module_exists('ls_answ')) {
    // Load file for the ls_answ_find_survey() function.
    module_load_include('inc', 'ls_answ', 'ls_answ');
    $node_survey = ls_answ_find_survey($node_type->type);
    // Load survey datas.
    module_load_include('inc', 'limesurvey_sync', 'ls_api/limesurvey_sync_api_survey');
    $survey_properties = limesurvey_sync_survey_properties($node_survey->ls_sid);
    $sid = $node_survey->ls_sid;
    $answ_type = $node_type->type;
    $answ_type_name = $node_type->name;
  }
  elseif ($node_type->module == 'ls_survey') {
    $answ_type_name = 'the_answers_content_type';
  }

  $redirect_method = array(
    'link' => t('print a simple hypertext link'),
    'form' => t('print a form button'),
    'redirect' => t('automatic and immediate redirection'),
    'iframe' => t('open into a iframe'),
    // @TODO : lightbox and modaliframe features still buggy.
    // Uncomment below lines to activate them.
    // 'lightbox' => t("print a link, opening a lightbox in front of the current page (need the '<a href=\"http://drupal.org/project/lightbox2\">lightbox2</a>' drupal custom module)"),
    // 'modaliframe' => t("open a lightbox in front of the current page (need the '<a href=\"http://drupal.org/project/modalframe\">Modal Frame API</a>' drupal custom module)"),
  );

  $form['ls_answers_options'] = array(
    '#type' => 'fieldset',
    '#title' => ($node_type->module == 'ls_survey') ? t('Default options for responses content types') : t('Options for this responses content type'),
    '#description' => ($node_type->module == 'ls_survey') ? t('Choose default display options for responses content types') : t('Choose display options for this responses content type'),
    '#weight' => -1,
    // Put the validate function into the hook_form_alter instead to make the
    // #node_type variable available for the validate function.
    //'#element_validate' => array('ls_survey_answ_options_validate'),
  );
  $form['ls_answers_options']['answ_title'] = array(
    '#type' => 'fieldset',
    '#title' => t('Options for the respond title'),
    '#description' => t('Choose how the title associated to each response will be created'),
    '#collapsible' => 1,
    '#collapsed' => 0,
  );
  $ls_prefix = (($node_type->module != 'ls_survey') ? $answer_options['ls_prefix'] : t('the_answers_content_type #'));
  $form['ls_answers_options']['answ_title']['ls_prefix'] = array(
    '#type' => 'textfield',
    '#title' => t('Prefix for the response title'),
    '#default_value' => $ls_prefix,
    '#description' => 'Delete the value for an empty title',
    '#required' => 0,
    '#size' => drupal_strlen($ls_prefix) + 10,
  );
  $form['ls_answers_options']['answ_title']['suffix'] = array(
    '#type' => 'fieldset',
    '#title' => t('Suffix'),
  );
  $suffix_method = array(
    'numeric' => t('An increment number'),
    'random' => t('A random number between :'),
  );
  $form['ls_answers_options']['answ_title']['suffix']['ls_suffix_method'] = array(
    '#type' => 'radios',
    '#title' => t('Suffix, to identify a single response'),
    '#default_value' => (!empty($sid) && $survey_properties['anonymized']) ? 'random' : $answer_options['ls_suffix_method'],
    '#options' => $suffix_method,
  );
  $form['ls_answers_options']['answ_title']['suffix']['ls_suffix_min'] = array(
    '#type' => 'textfield',
    '#title' => t('Minimal number'),
    '#default_value' => $answer_options['ls_suffix_min'],
    '#size' => '5',
  );
  module_load_include('inc', 'ls_answ', 'ls_answ'); // pour ls_answ_create_title()
  $form['ls_answers_options']['answ_title']['suffix']['ls_suffix_max'] = array(
    '#type' => 'textfield',
    '#prefix' => t('and'),
    '#title' => t('Maximal number'),
    '#default_value' => $answer_options['ls_suffix_max'],
    '#size' => '5',
  );
  // Answer title example
  if (module_exists('ls_answ')) {
    $answ_type_title = ($node_type->module != 'ls_survey') ? $answ_type : $answ_type_name;
    $form['ls_answers_options']['answ_title']['suffix']['ls_suffix_max']['#suffix'] = t('For example : %title', array('%title' => ls_answ_create_title($answ_type_title)));
  }
  $form['ls_answers_options']['answ_title']['ls_disabled_title'] = array(
    '#type' => 'checkbox',
    '#title' => t('The responses title can not be changed'),
    '#default_value' => (!empty($sid) && $survey_properties['anonymized']) ? 1 : $answer_options['ls_disabled_title'],
    '#return_value' => 1,
  );

  $form['ls_answers_options']['answ_redirect'] = array(
    '#type' => 'fieldset',
    '#title' => t('Options for the response completing form'),
    '#description' => t('Users submits their answers <em>on</em> the LimeSurvey website. Choose options about the completing form.'),
    '#collapsible' => 1,
    '#collapsed' => 0,
  );
  $message = t('In order to automatically close the frame, you should set on your LimeSurvey admin user interface :');
  $current_type = ($answ_type && !empty($form_state['values']['type']) && trim($form_state['values']['type']) != $type) ? $form_state['values']['type'] : $answ_type;
  $form['ls_answers_options']['answ_redirect']['ls_redirect'] = array(
    '#type' => 'radios',
    '#title' => t('Choose the way to redirect the user to the LimeSurvey site, in order to fill the survey'),
    '#default_value' => $answer_options['ls_redirect'],
    '#options' => $redirect_method,
    '#description' => $message . ls_survey_type_help_iframe($sid, $current_type, TRUE),
  );
  $form['ls_answers_options']['answ_redirect']['ls_validuntil'] = array(
    '#type' => 'select',
    '#title' => t('Choose expiration period for completing the survey'),
    '#default_value' => $answer_options['ls_validuntil'],
    '#description' => t('Since printing the survey to its final submission'),
    '#options' => array(
      600 => t('10 min'),
      3600 => t('1 hour'),
      7200 => t('2 hours'),
      86400 => t('1 day'),
      0 => t('Unlimited (not recommanded)'),
    ),
  );
  $end_url = array(
    'own' => t('its answer view page'),
    'page' => t('this specific page :'),
  );
  // In order to use the core sentence, remove double em tag.
  $right = filter_xss(t('%type_name: View own content', array('%type_name' => $answ_type_name)), array());
  $url_perm = url('admin/people/permissions');
  $form['ls_answers_options']['answ_redirect']['ls_end'] = array(
    '#type' => 'radios',
    '#title' => t('Once the form is submitted, redirect the user to'),
    '#description' => t('In order to allow a user to view its submission datas, you need to <b>set the appropiate rights</b> (%right) on the <a href="@url">permission setting page</a>', array('%right' => $right, '@url' => $url_perm)),
    '#default_value' => $answer_options['ls_end'],
    '#options' => $end_url,
  );
  $end_url_lenght = drupal_strlen($answer_options['ls_end_url']);
  $form['ls_answers_options']['answ_redirect']['ls_end_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Page to redirect after completion'),
    '#default_value' => $answer_options['ls_end_url'],
    '#description' => 'For example : node/458, leave it blank for the home page.',
    '#required' => 0,
    '#size' => ($end_url_lenght == 0) ? 20 : ($end_url_lenght + 10),
  );

  $form['ls_answers_options']['answ_options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Options for the response display page'),
    '#description' => t('Choose how datas will be displayed on the response page'),
    '#collapsible' => 1,
    '#collapsed' => 0,
  );
  $form['ls_answers_options']['answ_options']['ls_only_answered'] = array(
    '#type' => 'checkbox',
    '#title' => t('Print only responded questions'),
    '#description' => t('Usefull for large surveys'),
    '#default_value' => $answer_options['ls_only_answered'],
    '#return_value' => 1,
  );
  $form['ls_answers_options']['answ_options']['ls_merge'] = array(
    '#type' => 'checkbox',
    '#title' => t('Merge question groups, questions and subquestion for greater readability'),
    '#default_value' => $answer_options['ls_merge'],
    '#return_value' => 1,
  );
  $form['ls_answers_options']['answ_options']['ls_ignore_x'] = array(
    '#type' => 'checkbox',
    '#title' => t('Ignore "Text display" question type, also called "boilerplate questions"'),
    '#default_value' => $answer_options['ls_ignore_x'],
    '#return_value' => 1,
  );
  $ls_fields = array(
    'id' => t('Response ID'),
    'submitdate' => t('Date submitted'),
    'lastpage' => t('Last page'),
    'startlanguage' => t('Start language'),
    'token' => t('Token'),
  );

  if ($node_type->module != 'ls_survey' && module_exists('ls_answ')) {
    $ls_fields_date = $ls_fields_ipaddr = $ls_fields_refurl = array();
    if ($survey_properties['date_answer']) {
      $ls_fields_date = array(
        'datestamp' => t('Date last action'),
        'startdate' => t('Date started'),
      );
    }
    if ($survey_properties['ipaddr']) {
      $ls_fields_ipaddr = array(
        'ipaddr' => t('IP address'),
      );
    }
    if ($survey_properties['refurl']) {
      $ls_fields_refurl = array(
        'refurl' => t('Referrer URL'),
      );
    }
    $ls_fields = array_merge($ls_fields, $ls_fields_date, $ls_fields_ipaddr, $ls_fields_refurl);
  }

  $form['ls_answers_options']['answ_options']['ls_fields'] = array(
    '#type' => 'checkboxes',
    '#title' => t('LimeSurvey fields to be displayed'),
    '#default_value' => $answer_options['ls_fields'],
    '#options' => $ls_fields,
  );

  $form['ls_answers_options']['sync_options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Options for the response synchronization'),
    '#description' => t('Choose how manage datas during synchronization process'),
    '#collapsible' => 1,
    '#collapsed' => 0,
  );
  $auto_import_description = ($node_type->module != 'ls_survey') ? t('Or import answers manually on the <a href="@url">survey synchronization page</a>.', array('@url' => url('node/' . $node_survey->nid . '/import'))) : t('Or import answers manually on the survey synchronization page.');
  $form['ls_answers_options']['sync_options']['ls_auto_import'] = array(
    '#type' => 'checkbox',
    '#title' => t('Automatic import answer after completion'),
    '#description' => $auto_import_description,
    '#default_value' => $answer_options['ls_auto_import'],
    '#return_value' => 1,
  );
  $form['ls_answers_options']['sync_options']['ls_author'] = array(
    '#type' => 'textfield',
    '#title' => t('Set a specific user as author for the manually imported answers'),
    '#maxlength' => 60,
    '#size' => 40,
    '#autocomplete_path' => 'user/autocomplete',
    '#default_value' => $answer_options['ls_author'],
    '#description' => t('Leave it blank for the current user, or 0 for Anonymous user.'),
  );
  $form['ls_answers_options']['sync_options']['ls_standing_by_status'] = array(
    '#type' => 'checkbox',
    '#title' => t("Adapt the response status depending on the answer is completed or is being edited"),
    '#description' => t("Automatic switch to the 'unpublished' status during the edition process and switch back to the 'published' status when the response has been completed."),
    '#default_value' => $answer_options['ls_standing_by_status'],
    '#return_value' => 1,
  );
  $form['ls_answers_options']['sync_options']['ls_export_token'] = array(
    '#type' => 'checkbox',
    '#title' => t('Fill the token table of the LimeSurvey database with corresponding usernames and emails'),
    '#description' => t('Users names and emails from this site will be available into the LimeSurvey results.'),
    '#default_value' => $answer_options['ls_export_token'],
    '#return_value' => 1,
  );
  $form['ls_answers_options']['sync_options']['ls_keep_revisions'] = array(
    '#type' => 'checkbox',
    '#title' => t("Don't delete the answer revision entries store into the LimeSurvey site database"),
    '#description' => t('Keeping the answer revision entries allow you to switch back to this revision and being able to edit it again. They will be mark as "uncompleted reponses" into the LimeSurvey site user interface. you will need so to select "include completed answers only" to ignore them into the statitics.'),
    '#default_value' => $answer_options['ls_keep_revisions'],
    '#return_value' => 1,
  );
  $form['ls_answers_options']['sync_options']['ls_revision'] = array(
    '#type' => 'checkbox',
    '#title' => t('Always create revision on synchronization process'),
    '#default_value' => $answer_options['ls_revision'],
    '#return_value' => 1,
  );
  $form['ls_answers_options']['sync_options']['ls_recycling'] = array(
    '#type' => 'checkbox',
    '#title' => t('Recycling virgin responses for the same user'),
    '#description' => t('Virgin answers may be generated when the LimeSurvey form is displayed but the user did not submit the first page.'),
    '#default_value' => $answer_options['ls_recycling'],
    '#return_value' => 1,
  );

  $title_filter = t('Choose default format filter');
  if (user_access('administer filters')) {
    module_load_include('module', 'filter');
    // Overview of all formats.
    $options_filter = array();
    global $user;
    $formats = filter_formats($user);
    foreach ($formats as $id => $format) {
      $options_filter[$id] = t('@format_name (@format_code)', array('@format_name' => $format->name, '@format_code' => $id));
    }
    $form['ls_answers_options']['sync_options']['ls_format'] = array(
      '#type' => 'radios',
      '#title' => $title_filter,
      '#default_value' => $answer_options['ls_format'],
      '#description' => t('Only change this if you know what you are doing !'),
      '#options' => $options_filter,
    );
  }
  else {
    $form['ls_answers_options']['sync_options']['ls_format'] = array(
      '#type' => 'value',
      '#value' => $answer_options['ls_format'],
    );
    $form['ls_answers_options']['sync_options']['ls_format_display'] = array(
      '#type' => 'markup',
      '#prefix' => '<p>',
      '#markup' => '<strong>' . $title_filter . '</strong><br />--' . t('This option needs administrator access rights.') . '--',
      '#suffix' => '</p>',
    );
  }


  $form['ls_answers_options']['ls_options_last_import'] = array(
    '#type' => 'value',
    '#value' => $answer_options['ls_options_last_import'],
  );

  // The 'ls_survey_answ_options_submit()' function will be moved by the hook
  // form alter in order to be executed before the node_type_form_submit()
  // function.
  $form['#submit'][] = 'ls_survey_answ_options_submit';
  if ($node_type->module == 'ls_answ') {
    // Add specific redirection on the survey node.
    $form['#submit'][] = 'ls_survey_answ_options_redirect';
  }

  // The 'reset to default' button has been removed from D6 to D7 into the
  // node_type_form() function, we add it here.
  $form['actions']['reset'] = array(
    '#type' => 'submit',
    '#value' => t('Reset to defaults'),
    '#weight' => 50,
  );

  return $form;
}

/**
 * Validate the answers content type options.
 */
function ls_survey_answ_options_validate($form, &$form_state) {

  $node_type = $form['#node_type'];
  $frames = array('iframe', 'lightbox', 'modaliframe');
  $frames_set = in_array(trim($form_state['values']['ls_redirect']), $frames);
  $sid = $answ_type = FALSE;
  $variable_name = ($node_type->module == 'ls_survey') ? 'ls_survey_options' : 'ls_answ_options_' . check_plain($form_state['values']['old_type']);
  $old_settings = variable_get($variable_name, array());
  module_load_include('inc', 'limesurvey_sync', 'limesurvey_sync');

  if ($node_type->module != 'ls_survey' && module_exists('ls_answ')) {
    // Find the survey ID, answers content type and the correct url.

    // Load file for the ls_answ_find_survey() function.
    module_load_include('inc', 'ls_answ', 'ls_answ');
    $node_survey = ls_answ_find_survey($node_type->type);
    // Load survey datas.
    module_load_include('inc', 'limesurvey_sync', 'ls_api/limesurvey_sync_api_survey');
    $survey_properties = limesurvey_sync_survey_properties($node_survey->ls_sid);
    $sid = $node_survey->ls_sid;
    $answ_type = $node_type->type;

    // Create the correct url : for iframe, no_iframe and actual.
    // Adapt if the type has changed.
    $current_type = trim($form_state['values']['type']);
    $path = 'backfromsurvey/' . $current_type;
    $correct_back_url_no_iframe = url($path, array('absolute' => TRUE));
    $correct_back_url_iframe = $correct_back_url_no_iframe . '/back';
    $correct_back_url = ($frames_set) ? $correct_back_url_iframe : $correct_back_url_no_iframe;

    // Format the actual back url.
    $ls_settings = variable_get('ls_settings', array('ls_path' => ''));
    $url_ls = limesurvey_sync_rel2abs($survey_properties['#lang'][$node_survey->ls_lang]['back_url'], limesurvey_sync_rel2abs($ls_settings['ls_path'] . '/'));
    // Handle back url from LS, comparing to the drupal url() version.
    $actual_back_url = limesurvey_sync_drupal_formated_url($url_ls);

    if ($answ_type != $current_type) {
      // Rebuild the form to update the end URL on the 'answ_redirect'
      // field description. The form will be rebuild only if no error occurs.
      $form_state['rebuild'] = TRUE;
    }

  }

  // Validate all fields one by one.
  $keys = ls_survey_answ_default_options();
  foreach ($form_state['values'] as $key => $value) {
    if (array_key_exists($key, $keys)) {
      $tested_value = (!is_array($value)) ? trim($value) : $value;
      $condition = TRUE;
      $datas = $form;

      // Find fieldset :
      switch ($key) {
        case 'ls_options_last_import':
          $datas = $form['ls_answers_options'];
          break;
        case 'ls_prefix':
        case 'ls_disabled_title':
          $datas = $form['ls_answers_options']['answ_title'];
          break;
        case 'ls_suffix_method':
        case 'ls_suffix_min':
        case 'ls_suffix_max':
          $datas = $form['ls_answers_options']['answ_title']['suffix'];
          break;
        case 'ls_redirect':
        case 'ls_validuntil':
        case 'ls_end':
        case 'ls_end_url':
          $datas = $form['ls_answers_options']['answ_redirect'];
          break;
        case 'ls_only_answered':
        case 'ls_merge':
        case 'ls_ignore_x':
        case 'ls_fields':
          $datas = $form['ls_answers_options']['answ_options'];
          break;
        case 'ls_auto_import':
        case 'ls_author':
        case 'ls_standing_by_status':
        case 'ls_export_token':
        case 'ls_keep_revisions':
        case 'ls_revision':
        case 'ls_recycling':
        case 'ls_format':
          $datas = $form['ls_answers_options']['sync_options'];
          break;
      }

      // Check if datas are valid.
      switch ($datas[$key]['#type']) {
        case 'checkbox':
          $condition = in_array($tested_value, array('', 0, 1));
          if ($key == 'ls_disabled_title' && $tested_value && empty($form_state['values']['ls_prefix'])) {
            form_set_error($key, t('You can not disable an empty title.'));
          }
          elseif ($key == 'ls_disabled_title' && !$tested_value && !empty($sid) && $survey_properties['anonymized']) {
            form_set_error($key, t('For privacy raison, you can not change the answers title on anonymized surveys.'));
          }
          break;
        case 'select':
        case 'radios':
          $condition = array_key_exists($tested_value, $datas[$key]['#options']);
          break;
        case 'textfield':
          // Check the default value for this field.
          if (is_int($keys[$key])) {
            $condition = is_numeric($tested_value);
            if ($condition && $key == 'ls_suffix_max' && $tested_value <= $form_state['values']['ls_suffix_min']) {
              $condition = FALSE;
            }
          }
          break;
        case 'checkboxes':
          if (!is_array($tested_value)) {
            $condition = FALSE;
          }
          else {
            $condition = (array_diff_key($tested_value, $datas[$key]['#options']) == array());
          }
          break;
      }

      // Specific validations :
      if ($key == 'ls_options_last_import') {
        $condition = (empty($tested_value) || is_numeric($tested_value));
      }

      if ($key == 'ls_suffix_method' && $tested_value == 'numeric') {
        if ($sid && $survey_properties['anonymized']) {
          form_set_error($key, t('For privacy raison, you can choose only random title method on anonymized surveys.'));
        }
      }

      if ($key == 'ls_redirect' && $frames_set) {
        if ($tested_value == 'lightbox' && !module_exists('lightbox2')) {
          form_set_error($key, t("The '%module' module needs to be installed", array('%module' => 'lightbox2')));
        }
        elseif ($tested_value == 'modaliframe' && !module_exists('modalframe')) {
          form_set_error($key, t("The '%module' module needs to be installed", array('%module' => 'Modal Frame API')));
        }
      }

      if ($key == 'ls_author' && !empty($tested_value)) {
        $ls_author = db_query("SELECT 1 FROM {users} WHERE name = :name", array(':name' => $tested_value))->fetchField();
        if (!$ls_author) {
          form_set_error($key, t('Invalid user specified'));
        }
      }

      if ($key == 'ls_format' && $tested_value != $old_settings['ls_format'] && !user_access('administer filters')) {
        form_set_error($key, t('You are not not allowed to modify the filter format'));
      }

      // Survey properties specific validations.
      if ($sid) {

        if ($key == 'ls_redirect') {
          if ($frames_set && $actual_back_url != $correct_back_url_iframe) {
            form_set_error($key, t('In order to automatically close the frame, you should set on your LimeSurvey admin user interface :') . ls_survey_type_help_iframe($sid, $current_type, TRUE));
          }
          elseif (!$frames_set && $actual_back_url != $correct_back_url_no_iframe) {
            form_set_error($key, t('You should set on your LimeSurvey admin user interface :') . ls_survey_type_help_iframe($sid, $current_type, FALSE ));
          }
        }

        if ($key == 'ls_auto_import' && $tested_value) {
          if ($survey_properties['anonymized'] || !$survey_properties['date_answer']) {
            form_set_error($key, t('For privacy raison, anonymized or undated surveys can not automatically import answers'));
          }
          elseif ($actual_back_url != $correct_back_url) {
            form_set_error($key, t('In order to automatically import answers, you should set on your LimeSurvey admin user interface :') . ls_survey_type_help_iframe($sid, $current_type, $frames_set));
          }
          if ($survey_properties['tokenanswerspersistence'] && !$survey_properties['tokens_table']) {
            form_set_error($key, t('You need to !create the token table on the LimeSurvey admin interface in order to use the token persistence features', array('!create' => limesurvey_sync_link_to_ls(t('create'), $node_survey->ls_sid, 'editsurveysettings', 'tokens'))));
          }

        }

        if ($key == 'ls_export_token' && $tested_value && !$survey_properties['tokenanswerspersistence']) {
          form_set_error($key, t('Only surveys with token-based response persistence enabled can export users informations'));
        }

        if ($key == 'ls_keep_revisions' && $tested_value) {

          if (!$survey_properties['can_edit_answer']) {
            form_set_error($key, t('Only surveys that can edit answers may generate revisions'));
          }
          elseif (empty($form_state['values']['node_options']['revision'])) {
            form_set_error('node_options', t('You must enable revsionning in order to store the revisions entries'));
          }
        }

        if ($key == 'ls_recycling' && $tested_value && (!$survey_properties['tokenanswerspersistence'] || !$survey_properties['alloweditaftercompletion'])) {
          form_set_error($key, t('Only surveys with token-based response persistence enabled may generate virgin answers'));
        }

      }
      else {
        // This is the survey content type default page : don't allow to store
        // changes which can not be checked, because the default answer content
        // type variable is generated from this one.
        if ($key == 'ls_redirect' && in_array($tested_value, $frames)) {
          form_set_error($key, t('You can not edit the default redirection'));
        }

        if ($key == 'ls_auto_import' && $tested_value) {
          form_set_error($key, t('You can not choose the automatic import feature by default'));
        }

        if ($key == 'ls_export_token' && $tested_value) {
          form_set_error($key, t('You can not choose to export user informations by default'));
        }

        if ($key == 'ls_recycling' && $tested_value) {
          form_set_error($key, t('You can not choose to recycle virgin answers by default'));
        }

        if ($key == 'ls_keep_revisions' && $tested_value) {
          form_set_error($key, t('You can not choose to keep answers revisions by default'));
        }
      }

      if ($key == 'ls_end' && $tested_value == 'own' && !$form_state['values']['ls_auto_import']) {
        form_set_error($key, t("In order to be redirected to the answer view page, you need to enable the 'automatically import answers' feature"));
      }
      if ($key == 'ls_recycling' && $tested_value && !$form_state['values']['ls_validuntil']) {
        form_set_error('ls_validuntil', t('You can not ask for recycling answers with an unlimited expiration time'));
      }

      // Generate unspecific errors if needed.
      if (!$condition) {
        $option = $datas[$key]['#title'];
        form_set_error($key, t("The '%setting' setting is not valid.", array('%setting' => $option)));
      }
    }
  }
}

/**
 * Submit the answers content type options.
 *
 * This function creates or resets the associated answers content type and
 * saves options values into a drupal variable.
 */
function ls_survey_answ_options_submit($form, &$form_state) {

  // @TODO : Add a confirm page if local module is enable, checking if the
  // language has been set.

  if (is_numeric($form_state['values']['ls_author']) && $form_state['values']['ls_author'] == 0) {
    $author_uid = 0;
  }
  elseif (!empty($form_state['values']['ls_author'])) {
    $author_uid = db_query("SELECT uid FROM {users} WHERE name = :name", array(':name' => $form_state['values']['ls_author']))->fetchField();
  }
  else {
    $author_uid = '';
  }

  // Save options values and clean the $form_state['values'] array.
  // Initialisation.
  $new_settings = array();
  $default_options = ls_survey_answ_default_options();
  foreach ($form_state['values'] as $key => $value) {
    // Save only options values :
    if (array_key_exists($key, $default_options)) {
      if (!is_array($value)) {
        if ($key == 'ls_author') {
          $value = $author_uid;
        }
        $new_settings[$key] = check_plain($value);
      }
      else {
        $array_data = array();
        foreach ($value as $array_key => $array_value) {
          if (!empty($array_value)) {
            $array_data[] = check_plain($array_key);
          }
        }
        $new_settings[$key] = $array_data;
      }
      // Then delete values from the $form_state['values'] variable to prevent
      // those datas to be saved by the node_type_form_submit() function.
      unset($form_state['values'][$key]);
    }
  }

  // Manage the options variable before updating the node type :
  $node_type = $form['#node_type'];
  // Load appropriate variable, depending on the type.
  // Use "old_type" instead of "type" for the variable name : the new variable
  // name will be updated during ls_survey_node_type() function if needed.
  $variable_name = ($node_type->module == 'ls_survey') ? 'ls_survey_options' : 'ls_answ_options_' . check_plain($form_state['values']['old_type']);
  $old_settings = variable_get($variable_name, array());
  // If "reset to default" is requested, need to overwrite the new_settings.
  $op = isset($form_state['values']['op']) ? $form_state['values']['op'] : '';
  if ($op == t('Reset to defaults')) {
    $new_settings = $default_options;
    // Import 'ls_options_last_import' into the default value, to make sure
    // the comparison between old and new values will only involve all other
    // properties.
    if (isset($old_settings['ls_options_last_import'])) {
      $new_settings['ls_options_last_import'] = $old_settings['ls_options_last_import'];
    }
  }
  // The array_diff_assoc() function can not manage multidimensional array, so
  // we manage the ls_fields array separatly.
  $new_settings_strings = $new_settings;
  $old_settings_strings = $old_settings;
  unset($new_settings_strings['ls_fields'], $old_settings_strings['ls_fields']);
  $diff_settings = array_diff_assoc($new_settings_strings, $old_settings_strings);
  // Reintroduce the ls_field.
  if ($new_settings['ls_fields'] != $old_settings['ls_fields']) {
    $diff_settings['ls_fields'] = TRUE;
  }

  if (!empty($diff_settings)) {
    // Options settings have changed, they need to be updated.

    module_load_include('inc', 'ls_survey', 'ls_survey');
    if ($node_type->module == 'ls_answ') {
      $node_survey = ls_survey_find_survey($node_type->orig_type);
    }

    // Search for changes imposing to resynchronize all answers.
    // Load the options changes that impose resynchronized answers.
    $options_needing_resync = array(
      'ls_only_answered',
      'ls_merge',
      'ls_ignore_x',
      'ls_format',
      'ls_fields',
    );

    if (array_intersect_key($diff_settings, array_flip($options_needing_resync)) != array()) {
      $time = REQUEST_TIME;
      $new_settings['ls_options_last_import'] = $time;
      if ($node_type->module == 'ls_answ') {
        // All answers need to be resynchronized.
        drupal_set_message(t('You should <a href="@url">resynchronize all existing answers</a> in order to apply changes concerning the display page options', array('@url' => url('node/' . $node_survey->nid . '/import'))), 'warning');
      }
    }

    // Save the new settings
    variable_set($variable_name, $new_settings);
    // Watchdog the new settings :
    module_load_include('inc', 'limesurvey_sync', 'limesurvey_sync');
    limesurvey_sync_custom_watchdog($variable_name, $old_settings, $new_settings, 'ls_survey');

    // And delete answer entries on the LimeSurvey site if needed.
    if ($node_type->module == 'ls_answ') {
      if ($old_settings['ls_keep_revisions'] && !$new_settings['ls_keep_revisions'] && module_exists('ls_answ')) {
        drupal_set_message(t('You must <a href="@url">resynchronize all existing answers</a> in order to delete the obsolet revisions entries on your LimeSurvey site', array('@url' => url('node/' . $node_survey->nid . '/import'))), 'warning');
      }

      // And delete exported user entries on the LimeSurvey site if needed.
      if ($old_settings['ls_export_token'] && !$new_settings['ls_export_token'] && module_exists('ls_answ')) {
        drupal_set_message(t('You must <a href="@url">resynchronize all existing answers</a> in order to delete the exported users informations entries on your LimeSurvey site', array('@url' => url('node/' . $node_survey->nid . '/import'))), 'warning');
        // Processed to the test and updates.
        module_load_include('inc', 'ls_answ', 'ls_answ');
        $on = array("n.type = '%s'" => $form_state['values']['old_type']);
        $token_users_deleted = ls_answ_delete_exported_users($on);
        if ($token_users_deleted) {
          // Because we have used the access bypass, we don't explicit how many
          // answer entries have been updated.
          drupal_set_message(t('Exported user informations have been deleted on the LimeSurvey database.'));
        }
      }
    }
  }

  // Deal if reset to default requested.
  if ($op == t('Reset to defaults')) {

    if ($node_type->module == 'ls_answ' && $node_type->modified) {

      module_load_include('inc', 'ls_survey', 'ls_survey');
      $node_survey = ls_survey_find_survey($node_type->orig_type);
      if ($node_survey) {
        $default_type = ls_survey_default_node_type_answ($node_survey);
        // Replace the actual machine name by the orig type.
        // Because the node_type_save() function
        // uses the sql update : where type = ... we declare $info->old_type
        $default_type->old_type = check_plain($form_state['values']['old_type']);

        // We keep the orig_type by the old_type because the
        // node_type_reset() function need it.
        $default_type->orig_type = $default_type->type;

        // Delete "is_new" data for performance raisons. node_type_save
        // function will be call later via node_type_submit() function.
        unset($default_type->is_new);

        // The node_type_reset() function loads the reseted values from the
        // ls_answ_node_info() function. As the ls_answ_node_info() function
        // draws datas from the {node_type} table, we update it now.
        node_type_save($default_type);

        // Refresh the static variable node type
        node_types_clear();
        node_type_get_types();

        // Update $form_state['values']['old_type']
        $form_state['values']['old_type'] = $default_type->type;
      }
    }
  }
}

/**
 * Redirect after submitting the answers content type editing page.
 *
 * Redirect to the associated survey view after submitting the answers content
 * type editing page.
 */
function ls_survey_answ_options_redirect($form, &$form_state) {
  $node_type = $form['#node_type'];
  module_load_include('inc', 'ls_survey', 'ls_survey');
  $node_survey = ls_survey_find_survey($node_type->orig_type);
  if ($node_survey) {
    $form_state['redirect'] = 'node/' . $node_survey->nid;
  }

  // If asked for reset settings, overwrite the 'node_option_type' variable,
  // to delete the "promote" option. Executing here because the
  // node_type_form_submit() function has been executed already. That why we
  // use the type value and not the old_type value.
  $op = isset($form_state['values']['op']) ? $form_state['values']['op'] : '';
  if ($op == t('Reset to defaults')) {
    variable_set('node_options_' . $form_state['values']['type'], array("status"));
  }
}

/**
 * Display an html help text for setting the end url.
 *
 * @param $sid
 *   A numeric (optional). The survey ID reference. Default is NULL
 * @param $type
 *   A string (optional). The answer content type machine name. Default is
 *   FALSE corresponding to the text : 'your_answers_content_type_value'.
 * @param $iframes
 *   A Boolean (optional). If set to TRUE, display the end url for iframes
 *   feature (add '/back' to the end url). Default is FALSE.
 * @param $new_window
 *   A Boolean (optional). If set to TRUE, open links into a new window.
 *   Default is TRUE.
 *
 * @return
 *   An html string displaying helping text for configuring the end url.
 */
function ls_survey_type_help_iframe($sid = FALSE, $type = FALSE, $iframes = FALSE, $new_window = TRUE) {
  $type_value = ($type) ? $type : 'your_answers_content_type_value';
  $path = 'backfromsurvey/' . $type_value . (($iframes) ? '/back' : '');
  $url_backfromsurvey = url($path, array('absolute' => TRUE));

  // Load file for the limesurvey_sync_link_to_ls() function.
  module_load_include('inc', 'limesurvey_sync', 'limesurvey_sync');
  $load_url_tx = t('automatically load URL');
  $end_url_tx = t('End URL');
  $link_settings_autoredirect = (!empty($sid)) ? limesurvey_sync_link_to_ls($load_url_tx, $sid, 'editsurveysettings', 'presentation', TRUE, $new_window) : $load_url_tx;
  $link_settings_end_url = (!empty($sid)) ? limesurvey_sync_link_to_ls($end_url_tx, $sid, 'editsurveylocalesettings', 'edittxtele0', TRUE, $new_window) : $end_url_tx;

  $li = array();
  $li[] = t("'!link' to '%end_url' (or an equivalent relative url),", array('!link' => $link_settings_end_url, '%end_url' => $url_backfromsurvey));
  if ($iframes) {
    $show_autoredirect = TRUE;
    if ($sid) {
      module_load_include('inc', 'limesurvey_sync', 'ls_api/limesurvey_sync_api_survey');
      $survey_properties = limesurvey_sync_survey_properties($sid);
      if (isset($survey_properties['autoredirect']) && $survey_properties['autoredirect']) {
        $show_autoredirect = FALSE;
      }
    }
    if ($show_autoredirect) {
      $li[] = t("and '!link when survey complete ?' to <em>Yes</em> (optional).", array('!link' => $link_settings_autoredirect));
    }
  }
  return theme('item_list', array('items' => $li));
}
