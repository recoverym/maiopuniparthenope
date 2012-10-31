<?php
/**
 * @file
 * rules fonctions for the ls_answ module.
 */

/**
 * Implements hook_rules_condition_info().
 *
 * Create 2 events for the rule module : when an answer is completed and when a
 * answer is being edited.
 */
function ls_answ_rules_event_info() {
  // doc on http://drupal.org/node/298549
  return array(
    'ls_answ_sync_completed' => array(
      'label' => t('Response has been completed'),
      'group' => t('Ls answer'),
      'variables' => array(
        'node' => array(
          'type' => 'node',
          'label' => t('Answer'),
        ),
      ),
    ),
    'ls_answ_sync_editing' => array(
      'label' => t('Response is being edited'),
      'group' => t('Ls answer'),
      'variables' => array(
        'node' => array(
          'type' => 'node',
          'label' => t('Answer'),
        ),
      ),
    ),
    'ls_answ_sync_desynchronized' => array(
      'label' => t('Response is desynchronized'),
      'group' => t('Ls answer'),
      'variables' => array(
        'node' => array(
          'type' => 'node',
          'label' => t('Answer'),
        ),
      ),
    ),
    'ls_answ_sync_resynchronized' => array(
      'label' => t('Response is resynchronized'),
      'group' => t('Ls answer'),
      'variables' => array(
        'node' => array(
          'type' => 'node',
          'label' => t('Answer'),
        ),
      ),
    ),
  );
}
