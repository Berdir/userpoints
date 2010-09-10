<?php
// $Id$

/**
 * @file
 * Menu callbacks for userpoints.module.
 */


/**
 * Displays a detailed transaction report for an individual user.
 *
 * @param $uid
 *   User id, defaults to current user.
 */
function userpoints_list_my_userpoints($uid) {
  $overall_total = 0;
  $unapproved_total = 0;
  $approved_total = 0;

  $point_user = user_load($uid);

  $title = t('!Points for @username', userpoints_translation() + array('@username' => $point_user->name));
  drupal_set_title($title);

  //Grab all tids for this user.
  $sql = "SELECT p.tid, t.name
    FROM {userpoints_txn} p
    LEFT JOIN {taxonomy_term_data} t on p.tid = t.tid
    WHERE uid = :uid
    GROUP BY p.tid, t.name";
  $results = db_query($sql, array(':uid' => $uid));
  $grand_total = 0;
  foreach ($results as $result) {
    if ($result->name == NULL) {
      $result->name = t('!Uncategorized', userpoints_translation());
    }
    // Pull the sum from the caching table for resource reason and b/c the.
    $result->total = userpoints_get_current_points($uid, $result->tid);
    $args['subtotals'][$result->tid] = $result;

    // Maintain a grand total.
    $grand_total += $result->total;
  }
  $args['approved_total'] = $grand_total;

  // Grab the unmoderated point total.
  $args['unapproved_total'] = (int) db_query("SELECT SUM(points) FROM {userpoints_txn} WHERE uid = :uid AND status = 1", array(':uid' => $uid))->fetchField();
  $args['overall_total'] = ($args['approved_total'] + $args['unapproved_total']);

  $header = array(
      array('data' => t('!Points', userpoints_translation()), 'field' => 'points'),
      array('data' => t('Approved?'), 'field' => 'status'),
      array('data' => t('Date'), 'field' => 'time_stamp', 'sort' => 'desc'),
      array('data' => t('Operation'), 'field' => 'operation'),
      array('data' => t('Category'), 'field' => 'cat'),
      array('data' => t('Description'), 'field' => 'description'),
  );

  $query = db_select('userpoints_txn', 'p')->extend('PagerDefault')->extend('TableSort')
                  ->fields('p', array('txn_id', 'uid', 'time_stamp', 'points', 'operation', 'status', 'entity_type', 'entity_id', 'description'))
                  ->condition('p.uid', $uid)
                  ->orderByHeader($header)
                  ->limit(variable_get(USERPOINTS_REPORT_LIMIT, 10));

  $query->leftJoin('taxonomy_term_data', 't', 'p.tid = t.tid');
  $query->addField('t', 'name', 'cat');
  $stati = userpoints_txn_status();
  $num_rows = 0;
  $rows = array();
  foreach ($query->execute() as $row) {
    $num_rows++;
    $status = $stati[$row->status];
    if (!$row->cat) {
      $row->cat = t('!Uncategorized', userpoints_translation());
    }
    if (!empty($row->description)) {
      $description = $row->description;
    }
    else {
      $description = t('None');
    }
    $operations = userpoints_invoke_all('entity_type', $row);
    if (is_array($operations) && isset($operations[0])) {
      $operation = $operations[0];
    }
    else {
      switch ($row->entity_type) {
        case 'node':
          $node = node_load($row->entity_id);
          if ($node) {
            $operation = l($row->operation, 'node/' . $node->nid, array('attributes' => array('title' => $node->title)));
          }
          else {
            $operation = $row->operation;
          }
          break;
        case 'comment':
          if (module_exists('comment')) {
            // We have to load the comment to get the nid for the comment.
            $comment = comment_load($row->entity_id);
            if ($comment) {
              $operation = l($row->operation, 'node/' . $comment->nid, array('attributes' => array('title' => $comment->subject), 'fragment' => 'comment-' . $comment->cid));
            }
            else {
              $operation = $row->operation;
            }
          }
          break;
        default:
          $operation = $row->operation;
      }
    }
    $rows[] = array(
        array('data' => $row->points, 'align' => 'center'),
        array('data' => $status, 'align' => 'center'),
        array('data' => format_date($row->time_stamp, 'small'), 'align' => 'center'),
        array('data' => $operation),
        array('data' => $row->cat),
        array('data' => $description),
    );
    if ($num_rows <= 0) {
      $rows[] = array(
          array('data' => t('No !Points earned', userpoints_translation()), 'colspan' => 5, 'align' => 'center')
      );
    }
  }
  return theme('userpoints_list_my_userpoints', array('args' => $args, 'header' => $header, 'rows' => $rows));
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
