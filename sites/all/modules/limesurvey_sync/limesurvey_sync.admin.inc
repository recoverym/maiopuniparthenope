<?php
/**
 * @file
 * Administration of the limesurvey_sync module.
 */

/**
 * Form builder; administer limesurvey_sync module options.
 *
 * Set options for the LimeSurvey synchronization and display the testing
 * interface to check if parameters are valid, on the
 * "admin/config/ls_sync" page.
 *
 * @see limesurvey_sync_survey_admin_form_validate()
 * @see limesurvey_sync_survey_admin_form_submit()
 * @ingroup forms
 */
function limesurvey_sync_survey_admin_form($form, $form_state) {
  // Load default or actual settings.
  $default_ls_settings = array(
    'ls_path' => "http://{$_SERVER['HTTP_HOST']}/limesurvey",
    'ls_delta_time' => 0,
  );
  $settings =  variable_get('ls_settings', $default_ls_settings);

  // Use the provided values for the test interface.
  if (!empty($form_state['values'])) {
    // I'm not sure values have been sanitized throught the form process.
    $settings = array_map('check_plain', array_intersect_key($form_state['values'], $settings));
  }

  // Load testing interface.
  $form['testing'] = array(
    '#type' => 'fieldset',
    '#title' => t('Testing interface'),
    '#description' => t('Show testing results for synchronization between your Drupal site and your LimeSurvey site.')
    . '<br /><strong>' . t('Any test should fail.') . '</strong>',
    // Display the test interface on the top page.
    '#weight' => -1,
  );

  $form['testing']['results'] = array(
    '#type' => 'markup',
    '#prefix' => '<div>',
    '#markup' => limesurvey_sync_survey_admin_testing($settings),
    '#suffix' => '</div>',
  );

  // Load LimeSurvey synchronization input settings.
  $form['ls_survey'] = array(
    '#type' => 'fieldset',
    '#title' => t('LimeSurvey synchronization settings'),
  );

  $form['ls_survey']['ls_path'] = array(
    '#type' => 'textfield',
    '#title' => t('LimeSurvey site path'),
    '#default_value' => $settings['ls_path'],
    '#description' => t('It can be a relative path or an external website'),
    '#size' => (empty($settings['ls_path'])) ? 30 : (drupal_strlen($settings['ls_path']) + 10),
  );

  $form['ls_survey']['ls_delta_time'] = array(
    '#type' => 'textfield',
    '#title' => t('Adjust time between the Drupal server and the LimeSurvey server'),
    '#default_value' => $settings['ls_delta_time'],
    '#description' => t('in seconds'),
    '#size' => (drupal_strlen($settings['ls_delta_time']) + 5),
  );

  // Load buttons.
  $form['actions'] = array('#type' => 'actions');

  $form['actions']['test_button'] = array(
    '#type' => 'button',
    '#value' => t("Test settings (don't save)"),
  );

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

/**
 * Validate limesurvey_sync module options.
 */
function limesurvey_sync_survey_admin_form_validate($form, &$form_state) {
  // Validate the ls_path value.
  $error = FALSE;
  if (!isset($form_state['values']['ls_path']) || !is_string($form_state['values']['ls_path'])) {
    form_set_error('ls_path', t("The '%setting' setting is not valid.", array('%setting' => $form['ls_survey']['ls_path']['#title'])));
    $error = TRUE;
  }
  elseif (drupal_substr($form_state['values']['ls_path'], -1, 1) == '/') {
    form_set_error('ls_path', t('Delete trailing slash.'));
    $error = TRUE;
  }

  // Validate the ls_delta_time value.
  if (!isset($form_state['values']['ls_delta_time']) || (!empty($form_state['values']['ls_delta_time']) && !is_numeric($form_state['values']['ls_delta_time']))) {
    form_set_error('ls_delta_time', t("The '%setting' setting is not valid.", array('%setting' => $form['ls_survey']['ls_delta_time']['#title'])));
    $error = TRUE;
  }

  // Display specific error messages only if the 'save' button is clicked.
  if ($form_state['values']['op'] != t("Test settings (don't save)")) {

    // Test if all tests have passed.
    // I'm not sure values have been sanitized throught the form process.
    $settings = array_map('check_plain', array_intersect_key($form_state['values'], array_flip(array('ls_db_prefix', 'ls_path', 'ls_delta_time'))));
    module_load_include('inc', 'limesurvey_sync', 'limesurvey_sync');
    $result = limesurvey_sync_test_interface($settings);
    foreach ($result as $property => $datas) {
      if ($datas['result'] === FALSE) {
        $target = ($property == 'time') ? 'ls_delta_time' : ($property == 'url') ? 'ls_path' : '';
        form_set_error($target, $datas['message']);
      }
      elseif ($datas['result'] === NULL) {
        // Allow to save form inputs.
        drupal_set_message($datas['message'], 'warning');
      }
    }
  }
  elseif ($error) {
    // Only update the test interface display.
    // After submitting, don't use cache and rebuild the form in order to update
    // the test interface results with the provided values.
    // Note : If the user click on the 'save' button and a error is generated
    // during validation, the form will not be rebuild (form API limitation).
    // Not so big deal, the error message will provide the error information.
    $form_state['rebuild'] = TRUE;
  }
}

/**
 * Submit limesurvey_sync module options.
 */
function limesurvey_sync_survey_admin_form_submit($form, &$form_state) {
  if ($form_state['values']['op'] != t("Test settings (don't save)")) {
    $settings =  variable_get('ls_settings', array());
    // I'm not sure values have been sanitized throught the form process.
    $modified_settings = array_map('check_plain', array_intersect_key($form_state['values'], array_flip(array('ls_path', 'ls_delta_time'))));

    if ($modified_settings != $settings) {
      variable_set('ls_settings', $modified_settings);
      drupal_set_message(t('New settings have been saved.'));
      // Watchdog the new settings :
      module_load_include('inc', 'limesurvey_sync', 'limesurvey_sync');
      limesurvey_sync_custom_watchdog('ls_settings', $settings, $modified_settings);
    }
    else {
      drupal_set_message(t("Settings haven't been changed."));
    }
  }
}

/**
 * Displays results concerning tests on the LimeSurvey synchronization settings.
 *
 * Displayed, into an html table.
 *
 * @param $settings
 *   An array. Corresponding to the drupal variable ls_settings.
 *
 * @return
 *   The html table of the test result.
 */
function limesurvey_sync_survey_admin_testing($settings) {

  module_load_include('inc', 'limesurvey_sync', 'limesurvey_sync');
  $test_result = limesurvey_sync_test_interface($settings);

  // Create the $row variable to display test page as a html table.
  $row = $rows = array();
  foreach ($test_result as $property => $datas) {
    $row[] = array(
      'data' => $datas['title'],
      'header' => TRUE,
    );
    $row[] = limesurvey_sync_display_boolean_img($datas['result']);
    $row[] = $datas['message'];
    $row[] = $datas['help'];
    $rows[] = $row;
    $row = array();
  }

  $header_titles[] = array('data' => t('Tested property'));
  $header_titles[] = array('data' => t('Test result'));
  $header_titles[] = array('data' => t('Comment'));
  $header_titles[] = array('data' => t('Help'));
  $header = $header_titles;

  return theme('table', array('header' => $header, 'rows' => $rows));
}
