<?php
/**
 * @file
 * Page to redirecting to LimeSurvey answer fill page.
 */

/**
 * Page making the LimeSurvey available, as an iframe, or a link.
 *
 * If the answer node is not provided, create a link for a new answer node. The
 * form button is handle by the 'toform' page.
 *
 * @param $type
 *   A string. The answer content type.
 * @param $node
 *   An answer node object or FALSE if not provided
 *
 * @return
 *   A string. The url to fill the survey.
 */
function ls_answ_fill_survey($type, $node = FALSE) {
  // $type has been checked into the ls_answ_fill_survey_access() function.

  // Set the correct display mode.
  $display_mode = ls_answ_fix_display_mode($type);

  // Set the node.
  if (empty($node)) {
    module_load_include('inc', 'ls_answ', 'ls_answ');
    $node = ls_answ_new_node($type);
  }
  // Load LS survey properties
  module_load_include('inc', 'limesurvey_sync', 'ls_api/limesurvey_sync_api_survey');
  $survey_properties = limesurvey_sync_survey_properties($node->ls_sid);

  return ls_answ_linktosurvey($node, $display_mode);
}

/**
 * Manage link to the limeSurvey website for completing or editing an answer.
 *
 * This submit handler is called after the node saving process : it manage
 * http redirection if the user wants to modify the answers on the LimeSurvey
 * website. Options can alter redirection.
 *
 * @param $node
 *   An object. The answer node.
 * @param $display_mode
 *   A string. Informs how the Limesurvey website will be displayed. Available
 *   values are : "form", "redirect", 'iframe", "lightbox", "modaliframe",
 *   "link". If not provided, retireve the default display mode from the
 *   $node->type value.
 *
 * @return
 *   A string. The html code for displaying the Limesurvey answer editing form.
 */
function ls_answ_linktosurvey($node, $display_mode = '') {
  $output = '';
  module_load_include('inc', 'ls_survey', 'ls_survey.type');
  $options = variable_get('ls_answ_options_' . $node->type, ls_survey_answ_default_options());
  if (empty($display_mode)) {
    $display_mode = ls_answ_fix_display_mode($node->type);
  }
  module_load_include('inc', 'limesurvey_sync', 'ls_api/limesurvey_sync_api_survey');
  $survey_properties = limesurvey_sync_survey_properties($node->ls_sid);

  if (empty($node->nid) && $survey_properties['tokens_table']) {
    // Create an invitation on the LS database but don't save it on Drupal.
    module_load_include('inc', 'limesurvey_sync', 'ls_api/limesurvey_sync_api_answ');
    $node->ls_token = limesurvey_sync_answ_set_invit($node->type, $node->ls_sid, $node->ls_lang, '', '');
  }

  module_load_include('inc', 'limesurvey_sync', 'ls_api/limesurvey_sync_api_answ');
  $survey_url = limesurvey_sync_answ_surveyurl($node);

  switch ($display_mode) {

    case 'form':
      $nid = (!empty($node->nid)) ? '/' . $node->nid : $nid;
      drupal_goto('toform/' . $node->type . $nid);
      break;

    case 'redirect':
      drupal_goto($survey_url);
      break;

    case 'lightbox':
      // The custom module lightbox2 must be enable
      $output = '<a id="ls_lighframe" href="' . $survey_url . '" rel="lightframe[|width:900px;height:700px;]["' . $node->title . '"]">' . $node->title . '</a>';

    case 'iframe':
    case 'modaliframe':
      // This is the parent windows of the iframe.
      $custom_end_url = (!empty($options['ls_end_url'])) ? $options['ls_end_url'] : '<front>';
      // Skip the page loading the fresh answer if not needed.
      $end_url = ($options['ls_auto_import']) ? 'backfromsurvey/' . $node->type : $custom_end_url;
      $end_url = url($end_url, array('absolute' => TRUE));

      ls_answ_load_js_parent_page($display_mode, $end_url, $survey_url);
      if (!empty($node->nid)) {
        ls_answ_delete_success_message($node);
      }
      $output = theme('ls_answ_iframe_end.tpl', array('text' => l(t('Redirection in progress...'), $end_url)));
      // Do not print the iframe tag if display mode is lightbox or
      // modaliframe.
      if ($display_mode == 'iframe') {
        // Here, we can edit the iframe width and height properties.
        $output = theme('ls_answ_iframe.tpl', array('url' => $survey_url)) . $output;
      }
      return $output;
      break;

    case 'link':
    default:
      // Print a link to fill the form.
      module_load_include('inc', 'limesurvey_sync', 'ls_api/limesurvey_sync_api_answ');
      $output = '<h2 class="ls_answ_link_submit">';
      $output .= l(t('Go to the survey'), $survey_url);
      $output .= '</h2>';
      return $output;
  }
}

/**
 * Fire a JS fonction asking to the parent page to close the current child page.
 *
 * This page is going to be close thanks to its parent page.
 *
 * @param $type
 *   An string. The answer content type.
 *
 * @return
 *   A string. A waiting message.
 */
function ls_answ_close_frames($type) {
  // Set the correct display mode.
  $display_mode = ls_answ_fix_display_mode($type);


  // Load the associated survey node.
  module_load_include('inc', 'ls_answ', 'ls_answ');
  $node_survey = ls_answ_find_survey($type);

  // Insert javascrip stuff.
  $iframes = array('iframe', 'lightbox', 'modaliframe');
  if (in_array($display_mode, $iframes)) {
    // This is the child iframe, asking to its parent iframe (form/%/%node) to
    // disapear and to redirect to the drupal correct page
    // (backfromsurvey/%type).
    ls_answ_load_js_child_page($display_mode);
    // The js will take care of the redirection.
    return t('Redirection in progress...');
  }
}

/**
 * Load javascrip on the parent page when iframe is used to fill the survey.
 *
 * This function may auto open the frame displaying the LimeSurvey site on the
 * answer filling page, and prepare javascript to automatically close the
 * iframe and to redirect the user to a specific page on the Drupal page after
 * the answer has been completed.
 *
 * @param $display_mode
 *   A string. availables values are 'iframe', 'modaliframe', 'lightbox'.
 * @param $redirection_url
 *   A string. The url redirect after the survey has been completed. It may be
 *   the anser node view page.
 * @param $survey_url
 *   A string. LimeSurvey form url.
 * @param $redirection_timing
 *   A numeric. The duration before redirecting back to drupal. Default is
 *   100ms (=0.1s)
 */
function ls_answ_load_js_parent_page($display_mode, $redirection_url, $survey_url = '', $redirection_timing = 100) {
  $autoload_functions = array();
  $js_domain = $js_onresize = $js_custom = '';

  $js_redirect = "function closewindow() {
  document.getElementById('ls_form').style.display='none';
  document.getElementById('ls_form_end').style.display='inline';
  setTimeout(\"window.location='" . url($redirection_url, array('basolute' => TRUE)) . "'\", " . $redirection_timing . ");
};";

  switch ($display_mode) {
    case 'iframe':
      // Js code to improve domain conflicts.
      $js_domain = ls_answ_js_host();
      // Once the survey is finished : the frame disapears and the js
      // redirection beguins within 100ms.
      // From http://forums.digitalpoint.com/showthread.php?t=582469#post5546667.
      // Automatic resize the frame, from
      // http://www.dyn-web.com/tutorials/iframes/fluid.html
      $js_custom = 'function setIframeHeight(id, h) {
    var ifrm = document.getElementById(id);
    if (ifrm) {
        dw_Viewport.getWinHeight();
        ifrm.style.height = Math.round( h * dw_Viewport.height ) + "px";
        ifrm.style.marginTop = Math.round( (dw_Viewport.height -
            parseInt(ifrm.style.height) )/2 ) + "px";
    }
};';

      // Don't use the autoload function here because it will break the on load
      // calling 'callclosewindow' later (for un unknown raison).
      // $autoload_functions[] = "setIframeHeight('ifrm', .8);";
      $js_onresize = "window.onload = function() { setIframeHeight('ifrm', .8); }
  window.onresize = function() { setIframeHeight('ifrm', .8); }";
      break;

    case 'modaliframe':
      // @TODO : Page opening the LimeSurvey answering form via the Modal Frame
      // Module. Not tested yet, may need some work.
      // http://drupal.org/node/700752
      // Send the Modal Frame javascript for parent windows to the page.
      modalframe_parent_js();
      $js_custom = "function openframe() {Drupal.modalFrame.open('" . $survey_url . "')};";
      $autoload_functions[] = 'openframe';
      break;

    case 'lightbox':
      // @TODO : autoloading fails (infinit loop) ! As a workarround, a link
      // is displayed, clicking on it will open the lightbox, auto-open on IE
      // (FF fails). Automatic closing is functional but JS redirection fails.
      // $js_custom = "function openlsframe() {Lightbox.start('" . $survey_url . "', false, true, false, false)};";
      $js_custom = "function openlsframe() {document.getElementById('ls_lighframe').click()};";
      $autoload_functions[] = 'openlsframe';
      break;
  }

  // Autoload functions.
  $on_load = ls_answ_js_autoload_prepare($autoload_functions);

  // Load JS in the correct order.
  $js = array_diff(array($js_domain, $js_redirect, $js_custom, $on_load, $js_onresize), array());
  drupal_add_js(implode("\n", $js), array('type' => 'inline', 'scope' => JS_DEFAULT));
}

/**
 * Load javascrip on the child page when iframe is used to fill the survey.
 *
 * This function is loaded  after the answer has been completed on the
 * LimeSurvey site from an iframe. It asks to the parent page to close the
 * frame and to redirect the user to a specific page on the Drupal page after
 * the answer has been completed.
 *
 * @param $display_mode
 *   A string. Available values are 'iframe', 'modaliframe', 'lightbox'.
 *   Default is NULL : all js functions are loaded but they will be fired or
 *   not depending on the parent page, wich is correctly set.
 */
function ls_answ_load_js_child_page($display_mode = NULL) {
  // This is the child iframe, asking to its parent iframe (%node/form)
  // to disapear and to redirect to the drupal correct page (node/%node).
  // Help on http://drupal.org/node/17533#comment-150198.  $js_autoload = '';
  $js_domain = ls_answ_js_host();
  $autoload_functions = array('callclosewindow');
  $js_custom = 'function callclosewindow() {
    window.parent.closewindow();
};';

  if (module_exists('lightbox2') && ($display_mode == 'lightbox' || empty($display_mode))) {
    $js_custom .= "\n" . 'function callcloselightbox() {
    window.parent.Lightbox.end();
};';
    $autoload_functions[] = 'callcloselightbox';
  }
  if (module_exists('modalframe') && ($display_mode == 'modaliframe' || empty($display_mode))) {
    // Send the Modal Frame javascript for child windows to the page.
    modalframe_child_js();
    // Tell the parent window to close the modal frame dialog.
    modalframe_close_dialog();
  }

  // Autoload functions
  $on_load = ls_answ_js_autoload_prepare($autoload_functions);

  // Load JS in the correct order.
  $js = array_diff(array($js_domain, $js_custom, $on_load), array());
  drupal_add_js(implode("\n", $js), array('type' => 'inline', 'scope' => JS_DEFAULT));
}

/**
 * Autoload javascript functions on opening page.
 *
 * This function is loaded  after the answer has been completed on the
 * LimeSurvey site from an iframe. It asks to the parent page to close the
 * frame and to redirect the user to a specific page on the Drupal page after
 * the answer has been completed.
 *
 * @param $autoload_functions
 *   An array. Values are names of the javascrip functions to load
 *   automatically when the page opening.
 */
function ls_answ_js_autoload_prepare($autoload_functions = array()) {
  // Autoload functions.
  $js_autoload_func = $js_autoload = '';
  if (!empty($autoload_functions)) {
    $js_autoload_func = 'function addLoadEvent(func) {
  var oldOnload = window.onload;
  if (typeof window.onload != \'function\') {
    window.onload = func;
  }
  else {
    window.onload = function() {
      oldOnload();
      func();
    }
  }
};';

    // To launch some functions on loading.
    foreach ($autoload_functions as $function) {
      $js_autoload .= "\n" . 'addLoadEvent(' . $function . ');';
    }
  }
  // Load JS in the correct order.
  return $js_autoload_func . $js_autoload;
}

/**
 * Provide the servor domain name.
 *
 * Used to improve javascript communication between the Drupal site and an
 * iframe. Unfortunally, it seems to work only on localhost, this piece of js
 * code can produce the error: 'Illegal document.domain value' code: '1009'.
 * That why this function is disabled by default. Help on :
 * http://www.leggetter.co.uk/2010/03/12/making-cross-domain-javascript-
 * requests-using-xmlhttprequest-or-xdomainrequest.html
 *
 * @param $enable
 *   A boolean. If TRUE : send the server domain name. Default is FALSE.
 *
 * @return
 *   A string. The servor domain name.
 */
function ls_answ_js_host($enable = FALSE) {
  if (!$enable) {
    return '';
  }
  $host = $_SERVER['HTTP_HOST'];
  if (strpos($host, '.') !== FALSE) {
    // For 'www.mysite1.example.com' and 'www.mysite2.example.com':
    $darray = explode('.', $_SERVER['HTTP_HOST']);
    $narray = array_reverse($darray);
    // Return 'example'.
    $short_host = $narray[1];
  }
  else {
    $short_host = $host;
  }
  return "document.domain = '" . $short_host . "';";
}

/**
 * Delete the success message when editing or creating a node for esthetic raison.
 *
 * When an answer node is create or updated and the user is redirect on the
 * iframe, the success message is displayed but the user has not fill the
 * survey yet! Not so sexy, we delete it.
 *
 * @param $node
 *   An answer node object.
 */
function ls_answ_delete_success_message($node) {
  if (empty($node) || !is_object($node)) {
    return;
  }
  $t_args = array(
    '@type' => node_type_get_name($node),
    '%title' => $node->title,
  );
  $succesfull_insert = array('status' => array(t('@type %title has been created.', $t_args)));
  $succesfull_update = array('status' => array(t('@type %title has been updated.', $t_args)));

  $messages = drupal_get_messages();
  if (in_array($messages, array($succesfull_insert, $succesfull_update))) {
    drupal_get_messages(NULL, TRUE);
  }
}

/**
 * Make sure the display mode is correct.
 *
 * Usefull if some custom modules have been disabled but the options variable
 * has not been updated to reflect thoses changes.

 * @param $type
 *   A string. The answer content type machine name.
 *
 * @return
 *   A string, the correct display mode.
 */
function ls_answ_fix_display_mode($type) {
  module_load_include('inc', 'ls_survey', 'ls_survey.type');
  $default_options = ls_survey_answ_default_options();
  $options = variable_get('ls_answ_options_' . $type, $default_options);
  $display_mode = $options['ls_redirect'];
  if (empty($display_mode) || ($display_mode == 'lightbox' && !module_exists('lightbox2')) || ($display_mode == 'modaliframe' && !module_exists('modaliframe'))) {
    $display_mode = $default_options['ls_redirect']; // Default display mode.
  }
  return $display_mode;
}
