<?php
/**
 * @file
 * More functions for the limesurvey_sync module.
 */

/**
 * Provide link to specific pages of the LimeSurvey admin interface.
 *
 * @param $text
 *   An string. The link text.
 * @param $sid
 *   An numeric. Corresponding to the survey reference ID. Default is NULL.
 * @param $page
 *   A string. The page. Check the function for available values. Default is
 *   'admin'.
 * @param $anchor
 *   An string. The anchor value. Default is empty.
 * @param $absolute
 *   A boolean. For absolute url. Default is TRUE.
 * @param $new_windows
 *   A boolean. Open the LimeSurvey admin interface into a new window. Default
 *   is TRUE.
 *
 * @return
 *   A string, The corresponding url.
 */
function limesurvey_sync_link_to_ls($text, $sid = NULL, $page = 'admin', $anchor = '', $absolute = TRUE, $new_windows = TRUE) {
  $windows_tag = array(
    'attributes' => array(
      'title' => t('Open the LimeSurvey admin interface in a new window'),
      'onclick' => 'window.open(this.href); return false;',
    ),
  );
  $windows = ($new_windows) ? $windows_tag : array();
  module_load_include('inc', 'limesurvey_sync', 'ls_api/limesurvey_sync_api_sync');
  $url = limesurvey_sync_admin_url($sid, $page, $anchor, $absolute);
  return l($text, $url, $windows);
}

/**
 * Watchdog changes on array variables.
 *
 * This function watchdogs changes on array variables, expliciting changes. It
 * does not handle variable with numeric keys but it handles one level of
 * multidimensional arrays.
 *
 * @param $variable
 *   An string. Corresponding to the variable name.
 * @param $old_settings
 *   An array. Corresponding to variable values before changes.
 * @param $new_settings
 *   An array. Corresponding to variable values after changes (actual).
 * @param $module
 *   An string. Corresponding to the module name responsible for changes.
 * @param $severity
 *   An constant. Corresponding to the watchdog severity. Default to
 *   WATCHDOG_NOTICE.
 */
function limesurvey_sync_custom_watchdog($variable, $old_settings, $new_settings, $module = 'limesurvey_sync', $severity = WATCHDOG_NOTICE) {

  if ($new_settings != $old_settings) {
    $all_keys = array_merge($old_settings, $new_settings);
    foreach ($all_keys as $key => $change) {
      $values = array(
        '%variable' => $variable,
        '@field' => $key,
      );
      if (isset($new_settings[$key]) && isset($old_settings[$key])) {
        if ($old_settings[$key] != $new_settings[$key]) {
          $values['%old_val'] = (!is_array($old_settings[$key])) ? $old_settings[$key] : '(' . implode(', ', $old_settings[$key]) . ')';
          $values['%new_val'] = (!is_array($new_settings[$key])) ? $new_settings[$key] : '(' . implode(', ', $new_settings[$key]) . ')';
          // The first two watchdog() parameters should be literal strings.
          if ($module == 'limesurvey_sync') {
            watchdog('limesurvey_sync', 'The %variable settings have changed : @field from %old_val to %new_val', $values, $severity);
          }
          elseif ($module == 'ls_survey') {
            watchdog('ls_survey', 'The %variable settings have changed : @field from %old_val to %new_val', $values, $severity);
          }
        }
      }
      elseif (!isset($old_settings[$key]) && isset($new_settings[$key])) {
        $values['%val'] = (!is_array($new_settings[$key])) ? $new_settings[$key] : '(' . implode(', ', $new_settings[$key]) . ')';
        // The first two watchdog() parameters should be literal strings.
        if ($module == 'limesurvey_sync') {
          watchdog('limesurvey_sync', 'The %variable settings have changed : @field as been added (value is %val)', $values, $severity);
        }
        elseif ($module == 'ls_survey') {
          watchdog('ls_survey', 'The %variable settings have changed : @field as been added (value is %val)', $values, $severity);
        }
      }
      elseif (isset($old_settings[$key]) && !isset($new_settings[$key])) {
        $values['%val'] = (!is_array($old_settings[$key])) ? $old_settings[$key] : '(' . implode(', ', $old_settings[$key]) . ')';
        // The first two watchdog() parameters should be literal strings.
        if ($module == 'limesurvey_sync') {
          watchdog('limesurvey_sync', 'The %variable settings have changed : @field as been deleted (value was %val)', $values, $severity);
        }
        elseif ($module == 'ls_survey') {
          watchdog('ls_survey', 'The %variable settings have changed : @field as been deleted (value was %val)', $values, $severity);
        }
      }
    }
  }
}

/**
 * Inform if the LimeSurvey server is the not the same than the Drupal site one.
 *
 * Note : before using this function, the system must check if the LimeSurvey
 * database has been configured : isset($databases['ls']['default']) must
 * return TRUE.
 *
 * @return
 *   A boolean. TRUE if the server is not the same than the Drupal site one.
 */
function limesurvey_sync_external_server() {
  // Test if The LimeSurvey hosting server is the same that the drupal
  // hosting server.
  global $databases;
  // Copy the $database variable, keeping only the server informations.
  $server_keys = array(
    'driver' => '',
    'database' => '',
    'host' => '',
  );
  $bd_drupal = array_map('drupal_strtolower', array_intersect_key($databases['default']['default'], $server_keys));
  $bd_ls = array_map('drupal_strtolower', array_intersect_key($databases['ls']['default'], $server_keys));
  $diff = array_diff_assoc($bd_drupal, $bd_ls);
  return (!empty($diff));
}

/**
 * Calculate time difference between the Drupal server ad the LimeSurvey server.
 *
 * Both servers may not use the same time. This function calculates the
 * difference in seconds. Note : before using this function, the system must
 * check if the LimeSurvey database has been configured :
 * isset($databases['ls']['default']) must return TRUE.
 *
 * @return
 *   A sight numeric. The time difference in seconds or FALSE if the driver is
 *   not mysql, mysqli, pgsql or sqlite.
 */
function limesurvey_sync_calculate_server_time_diff() {
  // Test if The LimeSurvey hosting server is the same that the drupal
  // hosting server.
  global $databases;
  $ls_external_server = limesurvey_sync_external_server();

  if (!$ls_external_server) {
    // The most simple case, Drupal and Limesurvey use the same servor, time
    // should be the same.
    $delta_time = 0;
  }
  else {
    $servers = array('default', 'ls');
    // Load file for the limesurvey_sync functions.
    module_load_include('inc', 'limesurvey_sync', 'ls_api/limesurvey_sync_api_sync');
    foreach ($servers as $server) {
      if ($server == 'ls') {
        limesurvey_sync_db_open();
      }
      switch (drupal_strtolower($databases[$server]['default']['driver'])) {
        case 'mysqli':
        case 'mysql':
          $asking_for_the_date = 'SELECT UNIX_TIMESTAMP()';
          break;
        case 'pgsql':
          $asking_for_the_date = "SELECT date_part('epoch',now())";
          break;
        case 'sqlite':
          $asking_for_the_date = "SELECT strftime('%s','now')";
          break;
        default:
          return FALSE;
      }
      $date_on_server[$server] = db_query($asking_for_the_date)->fetchField();
      if ($server == 'ls') {
        limesurvey_sync_db_close();
      }
    }

    if ($date_on_server['ls'] != FALSE) {
      $delta_time = $date_on_server['default'] - $date_on_server['ls'];
    }
    else {
      $delta_time = 0;
    }
  }
  return $delta_time;
}

/**
 * Test if the LimeSurvey synchronization settings are valid.
 *
 * This function tests if the LimeSurvey synchronization settings are valid
 * and displays results into a multimensional array. It checks the connexion
 * to the LimeSurvey database, if the LimeSurvey tables exist on the database,
 * the LimeSurvey website connexion via http and the time difference between
 * both servers.
 *
 * @param $settings
 *   An array. Corresponding to the drupal variable ls_settings.
 *
 * @return
 *   A multimensional array providing results : Primary keys are 'server',
 *   'table', 'url', 'time' and, on each one, secondary keys are 'title' (a
 *   string, the property tested), 'result' (a boolean, TRUE if the test has
 *   succeeded), 'message' (a string, message to display), and 'help' : a
 *   string, the help text).
 */
function limesurvey_sync_test_interface($settings) {
  $test_result = array();

  // Test if The LimeSurvey hosting server has been configured.
  // Set message for some other tests.
  global $databases;
  $server_fails_comment = t('The LimeSurvey site hosting server test must succeed in order to run this test.');
  $server_fails_help = t('You need to fix the test of the LimeSurvey site hosting server first.');
  $server_ok = (isset($databases['ls']['default']));
  if ($server_ok) {
    $server_output = t('The LimeSurvey database connexion informations are configured');
    // Check regarding database prefix.
    $ls_external_server = limesurvey_sync_external_server();
    if (!$ls_external_server && $databases['ls']['default']['prefix'] == $databases['default']['default']['prefix']) {
      $server_ok = NULL;
      $server_output .= '<br /><strong>' . t('but the LimeSurvey database prefix is not valid, choose a value other than %invalid.', array('%invalid' => ((!empty($databases['ls']['default']['prefix'])) ? $databases['ls']['default']['prefix'] :  t('empty')))) . '</strong>';
    }
  }
  else {
    $server_output = '<strong>' . t('The LimeSurvey database connexion informations are not configured') . '</strong>';
  }

  $server_list = array();
  $server_list[] = '<strong>' . t("The LimeSurvey database connexion informations must be appended into the Drupal file <em>sites/default/settings.php</em>, adding the lines below after the \$databases['default']['default'] variable :") . '</strong>'
  . "\n\t\t" . '<br />' . '<blockquote>'
  . "\n\t\t" . "\$databases<strong>['ls']</strong>['default'] = array("
  . "\n\t\t" . '<br />' . "'driver' => 'your_LimeSurvey_driver',"
  . "\n\t\t" . '<br />' . "'database' => 'your_LimeSurvey_database_name',"
  . "\n\t\t" . '<br />' . "'username' => 'your_LimeSurvey_database_username',"
  . "\n\t\t" . '<br />' . "'password' => 'your_LimeSurvey_database_user_password',"
  . "\n\t\t" . '<br />' . "'host' => 'your_LimeSurvey_host'"
  . "\n\t\t" . '<br />' . "'prefix' => '<strong>lime_</strong>' [" . t('or another value') . "]"
  . "\n\t\t" . '<br />' . "'collation' => 'utf8_general_ci' [" . t('or another value') . "]"
  . "\n\t\t" . '</blockquote>'
  . "\n\t\t" . t('replacing each field by the correct value.');
  $server_list[] = t('You can find the correct <em>LimeSurvey database prefix</em> value opening the <em>config.php</em> file stored into your LimeSurvey site root folder (it is the <em>$dbprefix</em> variable value)');
  $server_list[] = t('Note that if you host your LimeSurvey site and your Drupal site on the same database, you have to choose distinct prefix.');
  $server_help = theme('item_list', array('items' => $server_list));

  $test_result['server'] = array(
    'title' => t('The LimeSurvey site hosting server'),
    'result' => $server_ok,
    'message' => $server_output,
    'help' => $server_help,
  );

  // Test if one LimeSurvey database table can be reached.
  if (!$server_ok) {
    $table_ok = NULL;
    $table_output = $server_fails_comment;
    $table_help = $server_fails_help;
  }
  else {
    module_load_include('inc', 'limesurvey_sync', 'ls_api/limesurvey_sync_api_sync');
    limesurvey_sync_db_open();
    $table_ok = db_table_exists(limesurvey_sync_table());
    limesurvey_sync_db_close();
    $table_output = ($table_ok) ? t("The 'surveys' database table of the LimeSurvey site has been successfully found") : t("The 'surveys' database table of the LimeSurvey site has not been found");
    $table_help = "\n" . t('If it fails, yo may need to check :');
    $table_list = array();
    $table_list[] = t('either your LimeSurvey database connexion informations,');
    $table_list[] = t('or the <em>LimeSurvey database prefix</em> value');
    $table_help .= theme('item_list', array('items' => $table_list));
  }

  $test_result['table'] = array(
    'title' => t('The LimeSurvey site database tables'),
    'result' => $table_ok,
    'message' => $table_output,
    'help' => $table_help,
  );

  // Test the LimeSurvey url.
  $url_base = limesurvey_sync_rel2abs($settings['ls_path']);
  // Need php5 for that testing headers values. Drupal 7 runs only under php5.
  // Help on :
  // http://robert.arles.us/2008/01/20/betwittered-php-and-file_get_contents/
  $windows_tag = array(
    'attributes' => array(
      'title' => t('Open the link into a new window'),
      'onclick' => 'window.open(this.href); return false;',
    ),
  );

  $test_manually = '<br />' . t('You need to manually test the !url URL, making sure you can reach your LimeSurvey site.', array('!url' => filter_xss(l($url_base, $url_base, $windows_tag))));
  if (!ini_get('allow_url_fopen')) {
    // To allow the treatment of URLs (like http:// or ftp://) as files.
    // ini_set('allow_url_fopen', 1);
    // Ini_set() function fails. The raison maybe because If Drupal set
    // something using php_admin_value in httpd.conf it is then not possible
    // to be set the value at runtime.
    $tested_url_output = t("<em>allow_url_fopen</em> should be set to '<em>On</em>' into the <em>php.ini</em> file for this test.")
    . '<br /><strong>' . t('Test abord.') . '</strong>' . $test_manually;
    $url_ok = NULL;
  }
  else {
    $ls_html = @file_get_contents($url_base);

    // Retrieve HTTP status code :
    if (!$ls_html) {
      // Some configurations fail here. As I don't know why : no FALSE. (False
      // break the form validation).
      $reached = NULL;
      $error_status = t('Undocumented error');
    }
    else {
      list($version, $status_code, $msg) = explode(' ', $http_response_header[0], 3);
      $reached = (($status_code >= 200 && $status_code <= 210) || ($status_code >= 300 && $status_code <= 310));
      // Check the HTTP Status code :
      if (!$reached) {
        switch ($status_code) {
          case 401:
          case 403:
            $error_status = t("Error 401: unauthorized request");
            break;
          case 400:
            $error_status = t("Error 400: invalid request");
            break;
          case 404:
            $error_status = t("Error 404: page not found");
            break;
          case ($status_code >= 500):
            $error_status = t("Error %num_error: server replied with an error", array('%num_error' => $status_code));
            break;
          default:
            $error_status = t("Undocumented error : @error", array('@error' => $status_code));
            break;
        }
      }
    }

    $url_admin = $url_base . '/admin/admin.php';
    $tested_url = ($reached) ? $url_admin : $url_base;
    $find_ls_text = FALSE;
    if ($reached) {
      $ls_html_admin = @file_get_contents($url_admin);
      // The word : "LimeSurvey" is on the footer aera in the login admin form
      // page.
      $find_ls_text = (($ls_html_admin !== FALSE && stripos($ls_html_admin, 'LimeSurvey') !== FALSE));
    }

    if ($reached && $find_ls_text) {
      $tested_url_output = t('The LimeSurvey site page has been successfully reached');
      $url_ok = TRUE;
    }
    elseif ($reached) {
      $tested_url_output = t("This page does not contain the word 'LimeSurvey'");
      $url_ok = FALSE;
    }
    else {
      $tested_url_output = t('The LimeSurvey site page has not been reached (%error)', array('%error' => $error_status));
      $url_ok = FALSE;
    }
    $tested_url_output .= '<br />' . t('Tested url is :<br />!url', array('!url' => filter_xss(l($tested_url, $tested_url, $windows_tag))));
  }

  $test_result['url'] = array(
    'title' => t('The LimeSurvey site url'),
    'result' => $url_ok,
    'message' => $tested_url_output,
    'help' => t('If it fails, yo may need to check the <em>LimeSurvey site path</em> setting and save it on the current page.'),
  );

  // Check for the date on both server
  if (!$server_ok) {
    $time_ok = NULL;
    $tested_delta_time_output = $server_fails_comment;
    $tested_delta_time_help = $server_fails_help;
  }
  else {
    $delta_time = limesurvey_sync_calculate_server_time_diff();
    $delta_time_saved = $settings['ls_delta_time'];
    $time_ok = ($delta_time !== FALSE) ? ($delta_time_saved == $delta_time) : NULL;
    $tested_delta_time_output = ($time_ok) ? t('Time on the Drupal server and on the LimeSurvey server is synchronized') :  t('Time on the Drupal server and on the LimeSurvey server is not synchronized');
    $tested_delta_time_help = t('If it fails, yo may set the <em>adjust time</em> setting to %delta seconds and save it on the current page.', array('%delta' => ($delta_time !== FALSE) ? $delta_time : t('undefined')));
  }

  $test_result['time'] = array(
    'title' => t('Synchronized servers'),
    'result' => $time_ok,
    'message' => $tested_delta_time_output,
    'help' => $tested_delta_time_help,
  );

  // Check for the date timezone on both server.
  $ls_gset_text = t('LimeSurvey global setting page');
  $link_ls_time = ($url_ok) ? limesurvey_sync_link_to_ls($ls_gset_text, NULL, 'globalsettings', 'general') : $ls_gset_text;
  if (!$server_ok) {
    $timezone_ok = NULL;
    $timezone_output = $server_fails_comment;
    $timezone_help = $server_fails_help;
  }
  else {
    // Here we check for of a LimeSurvey issue
    //(http://bugs.limesurvey.org/view.php?id=6038).
    // http://www.php.net/manual/en/function.date-default-timezone-get.php
    // LimeSurvey uses the code (on limesurvey/common.php line 89 :
    // @date_default_timezone_set(@date_default_timezone_get());
    // to set the timezone.

    if ($ls_external_server) {
      $timezone_ok = NULL;
      $timezone_output = t("The LimeSurvey server timezone can not be checked because your LimeSurvey site is hosted on an external server.")
    . '<br /><strong>' . t('Test abord.') . '</strong>' . t('Check it manually on the !link', array('!link' => $link_ls_time));

    }
    else {
      // Compare the drupal timezone and the server timezone, fixed by the
      // LimeSurvey global setting.
      $default_tz = ini_get('date.timezone');
      if (!$default_tz && version_compare(PHP_VERSION, '5.4.0', '<')) {
        // The timezone is "guess" from the host operating system.
        $timezone_ok = NULL;
        $timezone_output = t("The server timezone can not be checked.")
        . '<br /><strong>' . t('Test abord.') . '</strong>' . t('Check it manually on the !link', array('!link' => $link_ls_time));
      }
      else {
        $server_tz = ($default_tz) ? $default_tz : 'UTC';
        $server_tz_delta = strtotime(date("Y-m-d H:i:s") . ' ' . $server_tz) - strtotime(date("Y-m-d H:i:s"));
        limesurvey_sync_db_open();
        // Retrieve the limeSurvey timeadjust.
        $timeadjust = db_query('SELECT stg_value FROM {' . limesurvey_sync_table('settings_global') . '} WHERE stg_name = :stg_value', array(':stg_value' => 'timeadjust'))->fetchField();
        limesurvey_sync_db_close();
        // Format result from limesurvey/common.php line 331.
        // Set local time.
        if (drupal_substr($timeadjust, 0, 1) != '-' && drupal_substr($timeadjust, 0, 1) != '+') {
          $timeadjust = '+' . $timeadjust;
        }
        if (strpos($timeadjust, 'hours') === FALSE && strpos($timeadjust, 'minutes') === FALSE && strpos($timeadjust, 'days') === FALSE) {
          $timeadjust = $timeadjust . ' hours';
        }
        $ls_tz_delta = strtotime($timeadjust) - time();

        $delta_tz = $server_tz_delta - $ls_tz_delta;
        // Don't throw False, because this test is weak. Don't break the form
        // validation.
        $timezone_ok = !$delta_tz ? TRUE : NULL;
        $timezone_output = ($timezone_ok) ? t('Your LimeSurvey site timezone seems to be correctly set.') : t('Your LimeSurvey site timezone may not be not correctly set !') . '<br />' . t('The right setting may be %hours hours.', array('%hours' => number_format(($server_tz_delta / 3600) * gmp_sign($delta_tz)), 0));
      }
    }
  }

  $test_result['timezone'] = array(
    'title' => t('LimeSurvey site Timezone setting'),
    'result' => $timezone_ok,
    'message' => $timezone_output,
    'help' => t('If it fails, yo may check the <em>Time difference</em> setting on your !link.', array('!link' => $link_ls_time))
    . '<br />' . t('Note : informations from the LimeSurvey admin interface are the trusted ones.'),
  );

  return $test_result;
}

/**
 * Format an custom url in the same way as the url() function will do.
 *
 * This function is used to compare custom url with url generate by the url()
 * function. It takes care of the clean url. For example, convert the url :
 * 'http://www.example.com/?q=page' to 'http://www.example.com/page'.
 *
 * @param $url
 *   A string. The url to format.
 *
 * @return
 *   A string. The formated absolute url.
 */
function limesurvey_sync_drupal_formated_url($url) {
  $queries = array();
  if (strpos($url, '?') !== FALSE) {
    $parse_url = @parse_url($url);
    // Handle unique or multiple arguements.
    $parse_query = (strpos($parse_url['query'], '&') !== FALSE) ? explode('&', $parse_url['query']) : array($parse_url['query']);
    foreach ($parse_query as $arg) {
      $delimiter = strpos($arg, '=');
      if ($delimiter !== FALSE) {
        $queries[drupal_substr($arg, 0, $delimiter)] = drupal_substr($arg, $delimiter + 1);
      }
    }

    // Handle the q arguement.
    if (!empty($queries) && array_key_exists('q', $queries)) {
      $q_query = $queries['q'];
      unset($queries['q']);
      // Handle clean urls.
      if (variable_get('clean_url', '0') && array_key_exists('q', $queries)) {
        $parse_url['path'] .= $q_query;
      }
      else {
        // Need to set the q arg as the first arguement.
        $queries = array_merge(array('q' => $q_query), $queries);
      }
    }
    $scheme = (!empty($parse_url['scheme'])) ? $parse_url['scheme'] . '://' : '';
    $fragment = (!empty($parse_url['fragment'])) ? array('fragment' => $parse_url['fragment']) : array();
    $url = url($scheme . $parse_url['host'] . $parse_url['path'], array_merge(array('query' => $queries), $fragment));
  }
  // Return the absolute url.
  return $url;
}

/**
 * Convert relative url to absolute url.
 *
 * Code from http://stackoverflow.com/a/1243431.
 *
 * @param $rel
 *   A string. The relative url.
 * @param $base
 *   A string. The base url. Default is empty corresponding to the drupal site
 *   base url.
 *
 * @return
 *   A string, the converted absolute url.
 */
function limesurvey_sync_rel2abs($rel, $base = NULL) {
  // Set the base url.
  if (empty($base)) {
    global $base_url;
    $base = $base_url . '/';
  }

  if (empty($rel)) {
    return $base;
  }

  // Return if already absolute URL.
  // Removethe PHP_URL_SCHEME component and add @ to make this function
  // compatible with php < 5.1.2.
  $rel_scheme = @parse_url($rel);
  if (!empty($rel_scheme['scheme'])) {
    return $rel;
  }

  // Queries and anchors.
  if ($rel[0] == '#' || $rel[0] == '?') {
    return $base . $rel;
  }

  // Parse base URL and convert to local variables:  $scheme, $host, $path.
  extract(@parse_url($base));

  // Remove non-directory element from path.
  $path = preg_replace('#/[^/]*$#', '', $path);

  // Destroy path if relative url points to root.
  if ($rel[0] == '/') {
    $path = '';
  }

  // Dirty absolute URL.
  $abs = "$host$path/$rel";

  // Replace '//' or '/./' or '/foo/../' with '/'.
  $re = array('#(/\.?/)#', '#/(?!\.\.)[^/]+/\.\./#');
  for ($n = 1; $n > 0; $abs = preg_replace($re, '/', $abs, -1, $n)) {
  }
  // Absolute URL is ready!
  return (!empty($scheme)) ? $scheme . '://' . $abs : $abs;
}

/**
 * Return an image of a boolean value.
 *
 * @param $bool
 *   A boolean or the NULL value for 'warning'.
 * @param $center
 *   A boolean. If TRUE, align on center Default is TRUE.
 *
 * @return
 *   A string, the html image for TRUE, FALSE or WARNING.
 */
function limesurvey_sync_display_boolean_img($bool, $center = TRUE) {
  $align = ($center) ? ' align="center"' : '';
  $variable = array(
    'width' => 18,
    'height' => 18,
  );

  if ($bool === NULL) {
    $variable['path'] = 'misc/watchdog-warning.png';
    $variable['alt'] = $variable['title'] = t('Warning');
  }
  elseif ($bool) {
    $variable['path'] = 'misc/watchdog-ok.png';
    $variable['alt'] = $variable['title'] = t('Successful');
  }
  else {
    $variable['path'] = 'misc/watchdog-error.png';
    $variable['alt'] = $variable['title'] = t('Failure');
  }

  return '<div' . $align . '>' . theme('image', $variable) . '</div>';
}
