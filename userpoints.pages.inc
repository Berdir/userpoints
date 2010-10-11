<?php
// $Id$

/**
 * @file
 * Menu callbacks for userpoints.module.
 */


/**
 * Displays a detailed transaction report for an individual user.
 *
 * @param $account
 *   For which account to display.
 */
function userpoints_list_my_userpoints($account, $tid = NULL) {

  // Load the categories and check if there are multiples or only a single one.
  $categories = userpoints_get_categories();
  $multiple_categories = count($categories) > 1;

  // Store context in the output array so that modules have access to it.
  $output = array(
    '#account' => $account,
    '#tid' => $tid,
    '#attached' => array(
      'css' => array(
        drupal_get_path('module', 'userpoints') . '/userpoints_myuserpoints.css',
      ),
    ),
  );

  // Only display the category selection if there is more than one category
  // to select from. This check is specific to the categories the user has
  // points in.
  if (count(userpoints_get_categories($account)) > 1) {
    $output['form'] = drupal_get_form('userpoints_filter_cat_select', 'myuserpoints/' . $account->uid . '/', $tid, $account);
    $output['form']['#weight'] = -10;
  }

  // Prepare the table header.
  $header = array(
    array('data' => t('!Points', userpoints_translation()), 'field' => 'points', 'class' => array('userpoints-myuserpoints-header-points')),
    array('data' => t('Status'), 'field' => 'status', 'class' => array('userpoints-myuserpoints-header-status')),
    array('data' => t('Date'), 'field' => 'time_stamp', 'sort' => 'desc', 'class' => array('userpoints-myuserpoints-header-timestamp')),
    array('data' => t('Reason'), 'class' => array('userpoints-myuserpoints-header-reason')),
  );

  // Only display category if there is more than one category. In contrast to
  // the filter, this is not specific for the categories. If there are
  // categories, we want tell the user in which he has points, even if he
  // only has points in a single category.
  if ($multiple_categories) {
    $header[] = array('data' => t('Category'), 'field' => 'cat', 'class' => array('userpoints-myuserpoints-header-category'));
  }

  $query = db_select('userpoints_txn', 'p')->extend('PagerDefault')->extend('TableSort')
    ->fields('p')
    ->condition('p.uid', $account->uid)
    ->orderByHeader($header)
    ->limit(variable_get(USERPOINTS_REPORT_LIMIT, 10));

  $unapproved_query = db_select('userpoints_txn')
    ->condition('uid', $account->uid)
    ->condition('status', USERPOINTS_TXN_STATUS_PENDING);
  $unapproved_query->addExpression('SUM(points)');

  // Check for filtering. isset() is used because 0 is a valid value
  // (Uncategorized).
  if (isset($tid)) {
    // If a category is selected, limit both the default query and the query
    // that displays pending points to this category.
    $query->condition('p.tid', (int)$tid);
    $active_category = $categories[$tid];
    $unapproved_query->condition('tid', (int)$tid);
  }

  if (isset($active_category)) {
    drupal_set_title(t('!Points for @username (%category category)', userpoints_translation() + array('%category' => $active_category, '@username' => format_username($account))), PASS_THROUGH);
    $total_title = t('Total !points (%category category)', userpoints_translation() + array('%category' => $active_category));
  }
  else {
    drupal_set_title(t('!Points for @username', userpoints_translation() + array('@username' => format_username($account))));
    $total_title = t('Total !points', userpoints_translation());
  }

  $stati = userpoints_txn_status();
  $rows = array();
  foreach ($query->execute() as $transaction) {
    $status = $stati[$transaction->status];
    $row = array(
      array(
        'data' => $transaction->points,
        'class' => array('userpoints-myuserpoints-field-points'),
      ),
      array(
        'data' => $status,
        'class' => array('userpoints-myuserpoints-field-status'),
      ),
      array(
        'data' => format_date($transaction->time_stamp, 'small'),
        'class' => array('userpoints-myuserpoints-field-timestamp'),
      ),
      array(
        'data' => userpoints_create_description($transaction),
        'class' => array('userpoints-myuserpoints-field-reason'),
      ),
    );
    if ($multiple_categories) {
      $row[] = array(
        'data' => isset($categories[$transaction->tid]) ? $categories[$transaction->tid] : $categories[0],
        'class' => array('userpoints-myuserpoints-field-category'),
      );
    }
    $rows[] = $row;
  }

  $output['table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#empty' => t('No !Points earned', userpoints_translation()),
    '#weight' => -5,
    '#attributes' => array('class' => array('userpoints-myuserpoints-list')),
  );
  $output['pager'] = array(
    '#theme' => 'pager',
    '#weight' => 0,
  );

  // Fetch pending (not yet approved) points according to the category filter.
  $pending = (int)$unapproved_query
    ->execute()
    ->fetchField();

  // Display both pending and approved points in a simple table.
  $output['summary_table'] = array(
    '#theme' => 'table',
    '#header' => array(
      array(
        'data' => $total_title,
        'colspan' => 2,
      ),
    ),
    '#rows' => array(
      array(
        'data' => array(t('Approved !points', userpoints_translation()), userpoints_get_current_points($account->uid, isset($tid) ? $tid : 'all')),
        'class' => array('userpoints-myuserpoints-total-approved'),
      ),
      array(
        'data' => array(t('Pending !points', userpoints_translation()), $pending),
        'class' => array('userpoints-myuserpoints-total-pending'),
      ),
    ),
    '#weight' => 10,
    '#attributes' => array('class' => array('userpoints-myuserpoints-total')),
  );

  // For simplicity, the generated output is passed to a custom alter function.
  // This would also be possible through hook_page_alter(), but that hook is
  // hard to use.
  drupal_alter('userpoints_myuserpoints', $output);

  return $output;
}


/**
 * Lists the users and their point totals by all or by category.
 */
function userpoints_list_users($tid = NULL) {

  $header = theme('userpoints_list_users_header');
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

  $query->join('users', 'u', 'p.uid = u.uid AND u.status = 1');
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
  drupal_set_title(t("@category !points", userpoints_translation() + array('@category' => $cat)));

  if (variable_get(USERPOINTS_REPORT_DISPLAYZERO, 1) == 0) {
    // The user would NOT like to see users with zero points.
    $query->condition('p.points', 0, '<>');
  }

  $rows = array();
  foreach ($query->execute() as $data) {
    $rows[] = theme('userpoints_list_users_row', array('row' => $data));
  }
  return theme('userpoints_list_users', array('header' => $header, 'rows' => $rows, 'tid' => $tid));
}
