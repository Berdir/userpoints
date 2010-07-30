<?php
// $Id$

/**
 * @file
 * Provide better integration into the rules group
 */

/**
 * Implements hook_rules_data_type_info().
 */
function userpoints_rules_rules_data_type_info() {
  return array(
    'userpoints_transaction' => array(
      'label' => t('Userpoints transaction'),
      'property info' => array(
        'points' => array('label' => t('!Points', userpoints_translation()), 'type' => 'integer'),
        'tid' => array('label' => t('Userpoints category'), 'type' => 'integer'),
        'entity_type' => array('label' => t('Entity type'), 'type' => 'text'),
        'entity_id' => array('label' => t('Entity id'), 'type' => 'integer'),
        'description' => array('label' => t('Description'), 'type' => 'text'),
        'operation' => array('label' => t('Operation'), 'type' => 'text'),
      ),
      'hidden' => TRUE,
    ),
  );
}

/**
 * Implements hook_rules_action_info().
 */
function userpoints_rules_rules_action_info() {
  return array(
    'userpoints_action_grant_points' => array(
      'label' => t('Grant !points to a user', userpoints_translation()),
      'parameter' => array(
          'user' => array('type' => 'user', 'label' => t('User')),
          'points' => array('type' => 'integer', 'label' => t('!Points', userpoints_translation())),
          'tid' => array('label' => t('Userpoints category'), 'type' => 'integer'),
          'entity_type' => array('label' => t('Entity type'), 'type' => 'text'),
          'entity_id' => array('label' => t('Entity id'), 'type' => 'integer'),
          'description' => array('label' => t('Description'), 'type' => 'text'),
          'operation' => array('label' => t('Operation'), 'type' => 'text'),
        ),
      'group' => t('!Points', userpoints_translation()),
    ),
  );
}

/**
 * Implements hook_rules_event_info().
 */
function userpoints_rules_rules_event_info() {
  return array(
    'userpoints_event_points_awarded' => array(
       'label' => t('User was awarded !points', userpoints_translation()),
       'variables' => array(
          'user' => array('type' => 'user', 'label' => t('User')),
          'userpoints_transaction' => array('type' => 'userpoints_transaction', 'label' => t('Userpoints transaction'))
        ),
       'group' => t('!Points', userpoints_translation()),
    ),
  );
}

/**
 * Rules action - grant points to a user.
 */
function userpoints_action_grant_points($account, $points, $tid, $type, $id, $desc, $op) {
  userpoints_userpointsapi(array(
    'uid' => $account->uid,
    'points' => $points,
    'tid' => $tid,
    'entity_type' => $type,
    'entity_id' => $id,
    'description' => $desc,
    'operation' => $op,
  ));
}

/**
 * Rules action form configuration - allow number of points to be set.
 */
function userpoints_action_grant_points_form2($settings = array(), &$form) {
  $form['settings']['points'] = array(
    '#type' => 'textfield',
    '#title' => t('Number of points'),
    '#default_value' => isset($settings['points']) ? $settings['points'] : '',
    '#description' => t('The number of !points to award. You may enter a negative number as-well', userpoints_translation())
  );
  $form['settings']['description'] = array(
    '#type' => 'textfield',
    '#title' => t('Description'),
    '#default_value' => isset($settings['description']) ? $settings['description'] : '',
    '#description' => t('Points description')
  );
  $form['settings']['operation'] = array(
    '#type' => 'textfield',
    '#title' => t('Operation'),
    '#default_value' => isset($settings['operation']) ? $settings['operation'] : ''
  );
  $form['settings']['entity_type'] = array(
    '#type' => 'textfield',
    '#title' => t('Entity Type'),
    '#default_value' => isset($settings['entity_type']) ? $settings['entity_type'] : '',
    '#description' => t("Entity type. ex. 'node' or 'user'")
  );
  $form['settings']['entity_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Entity Id'),
    '#default_value' => isset($settings['entity_id']) ? $settings['entity_id'] : '',
    '#description' => t('ID of an entity in the Database. ex. $node->id or $user->uid')
  );
  //$form['settings']['tid'] = taxonomy_form(userpoints_get_vid(), isset($settings['tid']) ? $settings['tid'] : '', 'The category ID');
  return $form;
}


/**
 * Rules Conditions - compare userpoints with a defined amount
 */

/**
 * Implements hook_rules_condition_info().
 */
function userpoints_rules_rules_condition_info() {
  return array(
    'userpoints_rules_category' => array(
      'label' => t('Check the transaction category'),
      'parameter' => array(
        'userpoints_transaction' => array('type' => 'userpoints_transaction', 'label' => t('Userpoints transaction'))
      ),
      'group' => t('!Points', userpoints_translation()),
    ),
    'userpoints_rules_amount_before' => array(
      'label' => t('Compare Userpoints before the transaction'),
      'parameter' => array(
        'user' => array('type' => 'user', 'label' => t('User')),
        'userpoints_transaction' => array('type' => 'userpoints_transaction', 'label' => t('Userpoints transaction'))
      ),
      'group' => t('!Points', userpoints_translation()),
    ),
    'userpoints_rules_transaction_amount' => array(
      'label' => t('Compare the transaction amount'),
      'parameter' => array(
        'userpoints_transaction' => array('type' => 'userpoints_transaction', 'label' => t('Userpoints transaction'))
      ),
      'group' => t('!Points', userpoints_translation()),
    ),
    'userpoints_rules_amount' => array(
      'label' => t('Compare current Userpoints'),
      'parameter' => array(
        'user' => array('type' => 'user', 'label' => t('User')),
      ),
      'group' => t('!Points', userpoints_translation()),
    ),
  );
}

/**
 * Rules Condition form configuration - set the amount to compare
 */
function userpoints_rules_amount_form2($settings = array(), &$form, $use_category = TRUE) {
  if ($use_category) {
    $form['settings']['type'] = array(
      '#type' => 'select',
      '#title' => t('Userpoints category'),
      '#options' => userpoints_get_categories(),
      '#default_value' => isset($settings['type']) ? $settings['type'] : '',
      '#required' => TRUE,
    );
  }

  $form['settings']['amount'] = array(
    '#type' => 'textfield',
    '#title' => t('Amount to compare'),
    '#default_value' => isset($settings['amount']) ? $settings['amount'] : '',
    '#description' => t('The amount to compare if userpoints are >=. Example:30, will trigger the condition if the user userpoints are >= than 30 points.')
  );
}

/**
 * Condition: check current Userpoints
 */
function userpoints_rules_amount($account, $settings) {
  $balance = userpoints_get_current_points($account->uid, $settings['type']);
  return $balance >= $settings['amount'];
}

/**
 * Condition: check transaction amount
 */
function userpoints_rules_transaction_amount($transaction, $settings) {
  return $transaction['points'] >= $settings['amount'];
}

function userpoints_rules_transaction_amount_form2($settings = array(), &$form) {
  return userpoints_rules_amount_form($settings, $form, FALSE);
}

/**
 * Condition: check Userpoints before the transaction
 */
function userpoints_rules_amount_before($account, $transaction, $settings) {
  $balance = userpoints_get_current_points($account->uid, $settings['type']) - $transaction['points'];
  return $balance >= $settings['amount'];
}

function userpoints_rules_amount_before_form2($settings = array(), &$form) {
  return userpoints_rules_amount_form($settings, $form);
}

/**
 * Condition: check the transaction category
 */
function userpoints_rules_category($transaction, $settings) {
  return $transaction['tid'] == $settings['category'];
}

/**
 * Condition: check the transaction category configuration form
 */
function userpoints_rules_category_form2($settings = array(), &$form) {
  $form['settings']['category'] = array(
    '#type' => 'select',
    '#title' => t('Userpoints category'),
    '#options' => userpoints_get_categories(),
    '#default_value' => isset($settings['category']) ? $settings['category'] : '',
    '#required' => TRUE,
  );
}
