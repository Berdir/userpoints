<?php
// $Id$

/**
 * @file
 * Admin menu callbacks for userpoints.module.
 */

/**
 * Approves moderated points.
 */
function userpoints_admin_approve($operation, $txn_id) {
  return drupal_get_form('userpoints_confirm_approve', $operation, (int)$txn_id);
}

function userpoints_confirm_approve_submit($form, &$form_state) {
  global $user;

  switch ($form_state['values']['operation']) {
    case 'approve':
      $status = USERPOINTS_TXN_STATUS_APPROVED;
      break;
    case 'decline':
      $status = USERPOINTS_TXN_STATUS_DECLINED;
      break;
    default:
      return;
  }

  $params = array(
      'txn_id' => $form_state['values']['txn_id'],
      'approver_uid' => $user->uid,
      'status' => $status,
  );

  userpoints_userpointsapi($params);

  $form_state['redirect'] = 'admin/config/people/userpoints/moderate';
}


function userpoints_admin_txn($form, &$form_state, $mode, $txn_id) {
  global $user;

  $timestamp = format_date(REQUEST_TIME, 'custom', 'Y-m-d H:i O');

  if ($mode == 'edit' && $txn_id) {
    $txn = db_query('SELECT * FROM {userpoints_txn} WHERE txn_id = :txn_id', array(':txn_id' => $txn_id))->fetch();
    $timestamp = format_date($txn->time_stamp, 'custom', 'Y-m-d H:i O');
    $txn_user = user_load($txn->uid);
  }
  elseif ($mode == 'add' && $txn_id) {
    $txn_user = user_load($txn_id);
  }

  $form['txn_user'] = array(
      '#type' => 'textfield',
      '#title' => t('User Name'),
      '#size' => 30,
      '#maxlength' => 60,
      '#default_value' => isset($txn_user) ? $txn_user->name : '',
      '#autocomplete_path' => 'user/autocomplete',
      '#description' => t('User Name for the user you want the !points to affect', userpoints_translation()),
      '#required' => TRUE,
  );

  $form['points'] = array(
      '#type' => 'textfield',
      '#title' => t('Points'),
      '#size' => 10,
      '#maxlength' => 10,
      '#default_value' => isset($txn->points) ? $txn->points : 0,
      '#description' => t('Number of !points to add/subtract from the user. For example, 25 (to add !points) or -25 (to subtract !points).', userpoints_translation()),
      '#required' => TRUE,
  );

  $form['time_stamp'] = array(
      '#type' => 'textfield',
      '#title' => t('Date/Time'),
      '#default_value' => $timestamp,
      '#size' => 30,
      '#maxlength' => 30,
      '#description' => t('Date and time of this transaction, in the form YYYY-MM-DD HH:MM +ZZZZ'),
  );

  $expirydate = 0;
  if (isset($txn->txn_id)) {
    if ($txn->expirydate > 0) {
      $expirydate = format_date($txn->expirydate, 'custom', 'Y-m-d H:i O');
    }
  }
  else {
    // If we're not editing we use site defaults.
    $expirydate = userpoints_get_default_expiry_date();
    if ($expirydate) {
      $expirydate = format_date($expirydate, 'custom', 'Y-m-d H:i O');
    }
  }
  $form['expirydate'] = array(
      '#type' => 'textfield',
      '#title' => t('Expiration date'),
      '#default_value' => $expirydate,
      '#size' => 30,
      '#maxlength' => 30,
      '#description' => t('Date and time to expire these !points, in the form YYYY-MM-DD HH:MM +ZZZZ', userpoints_translation()) .
      '<br />' . t('Leave blank for non-expiring !points', userpoints_translation()),
  );
  if (module_exists('taxonomy')) {
    $form['tid'] = array(
        '#type' => 'select',
        '#title' => t('Category'),
        '#default_value' => variable_get(USERPOINTS_CATEGORY_DEFAULT_TID, 0),
        '#options' => userpoints_get_categories(),
        '#description' => t('Category to apply these !points to', userpoints_translation()),
    );
  }

  $form['reference'] = array(
      '#type' => 'textfield',
      '#title' => t('Reference'),
      '#default_value' => isset($txn->reference) ? $txn->reference : '',
      '#size' => 30,
      '#maxlength' => 128,
      '#description' => t('Enter optional reference for this transaction. This field will be indexed and searchable.'),
  );

  $form['description'] = array(
      '#type' => 'textarea',
      '#title' => t('Description'),
      '#default_value' => isset($txn->description) ? $txn->description : '',
      '#width' => 70,
      '#lines' => 5,
      '#description' => t('Enter an optional description for this transaction, such as the reason it is created.'),
  );


  switch ($mode) {
    case 'add':
      $form['approver_uid'] = array(
          '#type' => 'hidden',
          '#value' => $user->uid,
      );

      $form['operation'] = array(
          '#type' => 'hidden',
          '#value' => 'admin',
      );

      $form['status'] = array(
          '#type' => 'hidden',
          '#value' => USERPOINTS_TXN_STATUS_PENDING,
      );

      $form['mode'] = array(
          '#type' => 'hidden',
          '#value' => $mode,
      );
      break;

    case 'edit':

      $form['txn_user']['#disabled'] = TRUE;
      unset($form['txn_user']['#autocomplete_path']);

      $form['txn_uid'] = array(
          '#type' => 'value',
          '#value' => $txn->uid,
      );
      $form['txn_id'] = array(
          '#type' => 'value',
          '#value' => $txn_id,
      );
      $form['approver_uid'] = array(
          '#type' => 'textfield',
          '#description' => t('Approver ID'),
          '#default_value' => $txn->approver_uid,
          '#size' => 10,
          '#maxlength' => 7,
          '#description' => t('The user ID of the person who approved this transaction. 0 means not yet approved.'),
      );

      $form['operation'] = array(
          '#type' => 'textfield',
          '#description' => t('Operation'),
          '#default_value' => $txn->operation,
          '#size' => 20,
          '#maxlength' => 20,
          '#description' => t('The operation type for this transaction. Normally, it is "admin".'),
      );

      $form['status'] = array(
          '#title' => t('Approval status'),
          '#type' => 'radios',
          '#options' => userpoints_txn_status(),
          '#description' => t('Approval status of the transaction.'),
          '#default_value' => $txn->status,
      );
      break;
  }

  $form['mode'] = array(
      '#type' => 'hidden',
      '#default_value' => $mode
  );

  $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save'),
  );

  return $form;
}

function userpoints_admin_txn_submit($form, &$form_state) {
  if ($form_state['values']['form_id'] != 'userpoints_admin_txn') {
    return;
  }

  $txn_user = user_load_by_name($form_state['values']['txn_user']);
  if (!is_object($txn_user)) {
    return;
  }
  switch ($form_state['values']['mode']) {
    case 'add':
      $params = array(
          'points' => $form_state['values']['points'],
          'uid' => $txn_user->uid,
          'operation' => 'admin',
          'description' => $form_state['values']['description'],
          'reference' => $form_state['values']['reference'],
          'tid' => $form_state['values']['tid'],
          'time_stamp' => strtotime($form_state['values']['time_stamp']),
      );
      if ($form_state['values']['expirydate']) {
        // Check for the existence of an expirydate.
        $params['expirydate'] = strtotime($form_state['values']['expirydate']);
      }
      userpoints_userpointsapi($params);
      break;

    case 'edit':
      $expirydate = 0;
      if (!empty($form_state['values']['expirydate'])) {
        $expirydate = strtotime($form_state['values']['expirydate']);
      }
      $params = array(
          'uid' => $form_state['values']['txn_uid'],
          'approver_id' => $form_state['values']['approver_uid'],
          'points' => $form_state['values']['points'],
          'time_stamp' => strtotime($form_state['values']['time_stamp']),
          'operation' => $form_state['values']['operation'],
          'description' => $form_state['values']['description'],
          'reference' => $form_state['values']['reference'],
          'status' => $form_state['values']['status'],
          'expirydate' => $expirydate,
          'txn_id' => $form_state['values']['txn_id']
      );
      userpoints_userpointsapi($params);
  }

  $form_state['redirect'] = 'admin/config/people/userpoints';
}

/**
 * Provides an administrative interface for managing points.
 */
function userpoints_admin_points($tid = NULL) {
  $cat_count = count(userpoints_get_categories());

  $header = array(
      array('data' => t('User'), 'field' => 'u.name'),
      array('data' => t('Category'), 'field' => 't.name'),
      array('data' => t('!Points', userpoints_translation()), 'field' => 'p.points', 'sort' => 'desc'),
  );

  $query = db_select('userpoints', 'p')->extend('PagerDefault')->extend('TableSort')
                  ->fields('p', array('uid', 'points', 'tid'))
                  ->fields('u', array('name'))
                  ->groupBy('p.uid')
                  ->groupBy('u.name')
                  ->groupBy('p.points')
                  ->groupBy('p.tid')
                  ->groupBy('t.name')
                  ->orderByHeader($header)
                  ->limit(variable_get(USERPOINTS_REPORT_USERCOUNT, 30));

  $query->join('users', 'u', 'p.uid = u.uid');
  $query->leftJoin('taxonomy_term_data', 't', 'p.tid = t.tid');
  $query->addField('t', 'name', 'cat');

  // Check for filtering.
  if (!is_null($tid) && $tid == 0) {
    $query->condition('p.tid', 0);
    $cat = t('!Uncategorized', userpoints_translation());
  }
  elseif ($tid > 0) {
    $query->condition('p.tid', $tid);
    $cat = db_query("SELECT name from {taxonomy_term_data} WHERE tid = :tid", array(':tid' => $tid))->fetchField();
  }
  else {
    $cat = t('All');
  }
  drupal_set_title($cat . " " . t("!points", userpoints_translation()));

  if (variable_get(USERPOINTS_REPORT_DISPLAYZERO, 1) == 0) {
    // The user would NOT like to see users with zero points.
    $query->condition('p.points', 0, '<>');
  }

  $rows = array();
  foreach ($query->execute() as $data) {
    if (!$data->cat) {
      $data->cat = t('!Uncategorized', userpoints_translation());
    }
    $rows[] = array(
        array('data' => theme('username', array('account' => $data)) . "&nbsp;&nbsp;" . l(t("(details)"), "myuserpoints/$data->uid")),
        array('data' => $data->cat, 'align' => 'right'),
        array('data' => $data->points, 'align' => 'right'),
    );
  }

  // If there is only one category there is no sense in display the category filter dropdown.
  $output = array();
  if ($cat_count > 1) {
    $output = drupal_get_form('userpoints_filter_cat_select', 'admin/config/people/userpoints/', $tid);
  }
  $output['table'] = array(
    '#markup' => theme('table', array('header' => $header, 'rows' => $rows)),
  );
  $output['pager'] = array(
    '#markup' => theme('pager'),
  );
  return $output;
}


function userpoints_admin_manage($tid = NULL) {
  $cat_count = count(userpoints_get_categories());

  $header = array(
      array('data' => t('User'), 'field' => 'uid'),
      array('data' => t('Time stamp'), 'field' => 'time_stamp', 'sort' => 'desc'),
      array('data' => t('!Points', userpoints_translation()), 'field' => 'points'),
      array('data' => t('Operation'), 'field' => 'operation'),
      array('data' => t('Category'), 'field' => 'cat'),
      array('data' => t('Operation')),
  );

  $query = db_select('userpoints_txn', 'p')->extend('PagerDefault')->extend('TableSort')
                  ->fields('p', array('txn_id', 'uid', 'time_stamp', 'points', 'operation', 'status', 'entity_type', 'entity_id'))
                  ->condition('p.status', USERPOINTS_TXN_STATUS_PENDING)
                  ->orderByHeader($header)
                  ->limit(variable_get(USERPOINTS_REPORT_USERCOUNT, 30));

  $query->leftJoin('taxonomy_term_data', 't', 'p.tid = t.tid');
  $query->addField('t', 'name', 'cat');

  // Check for filtering.
  if (!is_null($tid) && $tid == 0) {
    $query->condition(db_or()->isNull('p.tid')->condition('p.tid', ''));
    $cat = t('!Uncategorized', userpoints_translation());
  }
  elseif ($tid > 0) {
    $query->condition('p.tid', $tid);
    $cat = db_query("SELECT name from {taxonomy_term_data} WHERE tid = :tid", array(':tid' => $tid))->fetchField();
  }
  else {
    $cat = t('All');
  }

  // Set the title of the page.
  // @todo: translation of $cat => t() is wrong.
  drupal_set_title(t('@category !points', userpoints_translation() + array('@category' => $cat)));
  $rows = array();
  foreach ($query->execute() as $data) {
    $account = user_load($data->uid);
    if (!$data->cat) {
      $data->cat = t('!Uncategorized', userpoints_translation());
    }
    $operations = userpoints_invoke_all('entity_type', $data);
    if (is_array($operations) && isset($operations[0])) {
      $operation = $operations[0];
    }
    else {
      switch ($data->entity_type) {
        case 'node' :
          $node = node_load($data->entity_id);
          if ($node) {
            $operation = l($data->operation, 'node/' . $node->nid, array('attributes' => array('title' => $node->title)));
          }
          else {
            $operation = $data->operation;
          }
          break;
        case 'comment' :
          if (module_exists('comment')) {
            //We have to load the comment to get the nid for the comment
            $comment = comment_load($data->entity_id);
            if ($comment) {
              $operation = l($data->operation, 'node/' . $comment->nid, array('attributes' => array('title' => $comment->subject), 'fragment' => 'comment-' . $comment->cid));
            }
            else {
              $operation = $data->operation;
            }
          }
          break;
        default:
          $operation = $data->operation;
      }
    }
    $rows[] = array(
        array('data' => theme('username', array('account' => $account))),
        array('data' => format_date($data->time_stamp, 'custom', 'Y-m-d H:i')),
        array('data' => $data->points, 'align' => 'right'),
        array('data' => $operation),
        array('data' => $data->cat),
        array('data' => l('approve', "admin/config/people/userpoints/approve/$data->txn_id") .
            ' ' . l('decline', "admin/config/people/userpoints/decline/$data->txn_id") .
            ' ' . l('edit', "admin/config/people/userpoints/edit/$data->txn_id")
        ),
    );
  }

  $output = array();
  if ($cat_count > 1) {
    $output = drupal_get_form('userpoints_filter_cat_select', 'admin/config/people/userpoints/moderate/', $tid);
  }
  $output['table'] = array(
    '#markup' => theme('table', array('header' => $header, 'rows' => $rows, 'empty' => t('No !points awaiting moderation', userpoints_translation()))),
  );
  $output['pager'] = array(
    '#markup' => theme('pager'),
  );
  return $output;
}


function userpoints_confirm_approve($form, $form_state, $operation, $txn_id) {
  $form = array(
      'operation' => array(
          '#type' => 'value',
          '#value' => $operation,
      ),
      'txn_id' => array(
          '#type' => 'value',
          '#value' => $txn_id,
      ),
  );

  return confirm_form(
          $form,
          t('Are you sure you want to @op txn @txn_id', array('@op' => $operation, '@txn_id' => $txn_id)),
          'admin/config/people/userpoints/moderate'
  );
}


/**
 * Menu callback for settings form.
 */
function userpoints_admin_settings($form, &$form_state) {
  $group = 'renaming';
  $form[$group] = array(
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#title' => t('Points branding'),
  );

  $form[$group][USERPOINTS_TRANS_UCPOINTS] = array(
      '#type' => 'textfield',
      '#title' => t('Word to use in the interface for the upper case plural word !Points', userpoints_translation()),
      '#default_value' => variable_get(USERPOINTS_TRANS_UCPOINTS, 'Points'),
      '#size' => 20,
      '#maxlength' => 20,
  );

  $form[$group][USERPOINTS_TRANS_LCPOINTS] = array(
      '#type' => 'textfield',
      '#title' => t('Word to use in the interface for the lower case plural word !points', userpoints_translation()),
      '#default_value' => variable_get(USERPOINTS_TRANS_LCPOINTS, 'points'),
      '#size' => 20,
      '#maxlength' => 20,
  );
  $form[$group][USERPOINTS_TRANS_UCPOINT] = array(
      '#type' => 'textfield',
      '#title' => t('Word to use in the interface for the upper case singular word !Point', userpoints_translation()),
      '#default_value' => variable_get(USERPOINTS_TRANS_UCPOINT, 'Point'),
      '#size' => 20,
      '#maxlength' => 20,
  );
  $form[$group][USERPOINTS_TRANS_LCPOINT] = array(
      '#type' => 'textfield',
      '#title' => t('Word to use in the interface for the lower case singular word !point', userpoints_translation()),
      '#default_value' => variable_get(USERPOINTS_TRANS_LCPOINT, 'point'),
      '#size' => 20,
      '#maxlength' => 20,
  );
  $form[$group][USERPOINTS_TRANS_UNCAT] = array(
      '#type' => 'textfield',
      '#title' => t('Word to use for the general category'),
      '#default_value' => variable_get(USERPOINTS_TRANS_UNCAT, 'General'),
      '#size' => 20,
      '#maxlength' => 20,
  );

  $group = 'status';
  $form[$group] = array(
      '#type' => 'fieldset',
      '#title' => t('Moderation'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#weight' => -1,
  );

  $form[$group][USERPOINTS_POINTS_MODERATION] = array(
      '#type' => 'radios',
      '#title' => t('Transaction status'),
      '#default_value' => variable_get(USERPOINTS_POINTS_MODERATION, 0),
      '#options' => array(t('Approved'), t('Moderated')),
      '#description' => t('Select whether all !points should be approved automatically, or moderated, and require admin approval', userpoints_translation()),
  );

  $group = "Points expiration";
  $form[$group] = array(
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#title' => t('!Points expiration', userpoints_translation()),
      '#description' => t('These settings affect new !points only, they are not retroactive. !Points expiration depends upon cron.', userpoints_translation()),
  );

  $form[$group][USERPOINTS_EXPIREAFTER_DATE] = array(
      '#type' => 'select',
      '#title' => t('Expire !points after', userpoints_translation()),
      '#description' => t('Once !points have been obtained by the user
                          they will expire according to this setting', userpoints_translation()),
      '#options' => expiry_dates(),
      '#default_value' => variable_get(USERPOINTS_EXPIREAFTER_DATE, NULL),
  );

  // If the expiration date is earlier than today/ new points will last forever.
  // Although this may be desirable/ it could also be an oversight so we'll
  // display a message to the administrator.
  $warning = "";
  if (userpoints_date_to_timestamp(variable_get(USERPOINTS_EXPIREON_DATE, array('day' => 1, 'month' => 1, 'year' => 1900))) < REQUEST_TIME) {
    $warning = '<br /><strong>' . t('This setting will not take affect, date must be in the future') . '</strong>';
  }

  $form[$group][USERPOINTS_EXPIREON_DATE] = array(
      '#type' => 'date',
      '#title' => t('Expire !points on this date', userpoints_translation()),
      '#description' => t('Once !points have been obtained by the user they will
                         last until this date. This setting overrides the
                         "Expire after setting" above ', userpoints_translation()) . $warning,
      '#default_value' => variable_get(USERPOINTS_EXPIREON_DATE, array('day' => 1, 'month' => 1, 'year' => 1980)),
  );
  $form[$group][USERPOINTS_EXPIRY_DESCRIPTION] = array(
      '#type' => 'textarea',
      '#title' => t('Expiration entry description'),
      '#description' => t('A negating expiration entry is made to expire
                         !points leaving the original entry intact
                         (e.g. original !points + expiration !points = 0).
                         When the expiration entry is made this description will
                         be placed on the entry. This is useful so the users will
                         know what happened to their !point balance. In crafting
                         your message you can use the following variables.',
              userpoints_translation()) . t('
                         <br /> !points = The name used in branding
                         above (also use !Points and !point)') . t('
                         <br /> !operation = Original operation that granted the !points
                         <br /> !description = Original description for the !point
                         <br /> !txn_id Original transaction ID
                         <br /> !date = Date of the original entry', userpoints_translation()),
      '#default_value' => variable_get(USERPOINTS_EXPIRY_DESCRIPTION, ''),
  );

  $group = "misc";
  $form[$group] = array(
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#title' => t('Messages'),
      '#description' => t('Control the behavior of messages users see.'),
  );

  $form[$group][USERPOINTS_DISPLAY_MESSAGE] = array(
      '#type' => 'radios',
      '#title' => t('Display message'),
      '#default_value' => variable_get(USERPOINTS_DISPLAY_MESSAGE, 1),
      '#options' => array(0 => t('No'), 1 => t('Yes')),
      '#description' => t('Determines if a message should be displayed whenever !points are awarded/substracted.', userpoints_translation()),
  );
  $group = "reports";
  $form[$group] = array(
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#title' => t('Report Settings'),
      '#description' => t(''),
  );

  $form[$group][USERPOINTS_REPORT_LIMIT] = array(
      '#type' => 'select',
      '#title' => t('Transactions per page'),
      '#default_value' => variable_get(USERPOINTS_REPORT_LIMIT, 10),
      '#options' => array(10 => 10, 20 => 20, 30 => 30, 40 => 40, 50 => 50, 100 => 100),
      '#description' => t('Limits the number of transactions displayed per page.'),
  );
  $form[$group][USERPOINTS_REPORT_DISPLAYZERO] = array(
      '#type' => 'radios',
      '#title' => t('Display zero !point users?', userpoints_translation()),
      '#default_value' => variable_get(USERPOINTS_REPORT_DISPLAYZERO, 1),
      '#options' => array(t('No'), t('Yes')),
      '#description' => t('If set to "No" users with zero !points will not be displayed in the reports', userpoints_translation()),
  );
  $form[$group][USERPOINTS_REPORT_USERCOUNT] = array(
      '#type' => 'select',
      '#title' => t('Users per page'),
      '#default_value' => variable_get(USERPOINTS_REPORT_USERCOUNT, 30),
      '#options' => array(10 => 10, 20 => 20, 30 => 30, 40 => 40, 50 => 50, 100 => 100),
      '#description' => t('When listing !points by user limit how many users are displayed on a single page', userpoints_translation()),
  );


  // Categories will only appear if the taxonomy module is enabled as
  // the module is required for this functionality but not necessarily
  // a requirement for the module.
  if (module_exists('taxonomy')) {
    $group = 'category';
    $form[$group] = array(
        '#type' => 'fieldset',
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
        '#title' => t('!Points Categorization', userpoints_translation()),
    );
    $form[$group][USERPOINTS_CATEGORY_DEFAULT_TID] = array(
        '#type' => 'select',
        '#title' => t('Default Category'),
        '#default_value' => variable_get(USERPOINTS_CATEGORY_DEFAULT_TID, NULL),
        '#options' => userpoints_get_categories(),
        '#description' => t('By default all !points are assigned to this category.  You can modify what categories are available by modifying the <a href="!url">Userpoints taxonomy</a>',
                array_merge(userpoints_translation(), array('!url' =>  url('admin/structure/taxonomy/' . taxonomy_vocabulary_load(variable_get(USERPOINTS_CATEGORY_DEFAULT_VID, ''))->machine_name)))),
    );
    $options = userpoints_get_categories();
    // $options doesn't include all so we'll add it.
    $options['all'] = t('All categories');
    $form[$group][USERPOINTS_CATEGORY_PROFILE_DISPLAY_TID] = array(
        '#type' => 'select',
        '#title' => t('Category to display on the user profile page'),
        '#default_value' => variable_get(USERPOINTS_CATEGORY_PROFILE_DISPLAY_TID, NULL),
        '#options' => $options,
        '#description' => t('Select which category of !points to display on the user profile page. Select "All" to display a sum total of all categories', userpoints_translation()),
    );
  }
  // New configuration options to overide current timestamp.
  $group = "stamping";
  $form[$group] = array(
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#title' => t('Transaction stamping'),
      '#description' => t(''),
  );
  $form[$group][USERPOINTS_TRANSACTION_TIMESTAMP] = array(
      '#type' => 'checkbox',
      '#title' => t('Always use system time'),
      '#default_value' => variable_get(USERPOINTS_TRANSACTION_TIMESTAMP, 1),
      '#description' => t('Sets if the transaction timestamp should obey current time, or can be modified by the API operations. Unchecking this option will allow customization of timetamp for the transactions.'),
  );

  $form['setting'] = userpoints_invoke_all('setting');
  return system_settings_form($form);
}
