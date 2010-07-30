<?php
// $Id$

/**
 * @file
 * Theme functions for userpoints.module
 */

/**
 * Allows my_userpoints to be themeable.
 */
function theme_userpoints_list_my_userpoints($attributes) {
  $args = $attributes['args'];
  $header = $attributes['header'];
  $rows = $attributes['rows'];
  $output = theme('table', array('header' => $header, 'rows' => $rows));
  $output .= "<p>";
  if (isset($args) && isset($args['subtotals'])) {
    foreach ($args['subtotals'] as $tid => $data) {
      $output .= '<strong>' . $data->name . ' ' . t('!points Balance', userpoints_translation()) . ':</strong> ' . $data->total . '<br />';
    }
  }
  $output .= "</p>";
  $output .= "<p>";
  $output .= '<strong>' . t('Approved !points Balance', userpoints_translation()) . ':</strong> ' . $args['approved_total'] . '<br />';
  $output .= '<strong>' . t('!Points awaiting moderation', userpoints_translation()) . ':</strong> ' . $args['unapproved_total'] . '<br />';
  $output .= '<strong>' . t('Net !points Balance', userpoints_translation()) . ':</strong> ' . $args['overall_total'] . '<br />';
  $output .= "</p>";
  $output .= theme('pager');

  return $output;
}

/*
 * Themes the output of users by points page.
 *
 * @return
 *   HTML of the table and a pager
 *
 * @see theme_userpoints_list_users_rows()
 */
function theme_userpoints_list_users($attributes) {
  $header = $attributes['header'];
  $rows = $attributes['rows'];
  $tid = $attributes['tid'];

  // If there is only one category, there is no sense in display the category filter dropdown
  $output = array();
  if (count(userpoints_get_categories()) > 1) {
    $output = drupal_get_form('userpoints_filter_cat_select', 'userpoints/', $tid);
  }
  $output['table'] = array(
    '#markup' => theme('table', array('header' => $header, 'rows' => $rows)),
  );
  $output['pager'] = array(
    '#markup' => theme('pager'),
  );
  return $output;
}

/**
 * Themes the header of the table on the "user by points" page.
 *
 * @return
 *   An header array used by theme_table() and TableSort.
 */
function theme_userpoints_list_users_header() {
  return array(
      array('data' => t('User'), 'field' => 'u.name'),
      array('data' => t('Category'), 'field' => 't.name'),
      array('data' => t('!Points', userpoints_translation()), 'field' => 'p.points', 'sort' => 'desc'),
  );
}

/*
 * Themes the output of a single row of the "user by points" page
 *
 * @return
 *   Array for a single row, suitable for inclusion with theme_table.
 */
function theme_userpoints_list_users_row($attributes) {
  global $user;

  $row = $attributes['row'];

  if (!$row->cat) {
    $row->cat = t('!Uncategorized', userpoints_translation());
  }

  $details = '';
  if ($user->uid == $row->uid) {
    $details = "&nbsp;&nbsp;" . l(t("(details)"), "myuserpoints");
  }

  return array(
      array('data' => theme('username', array('account' => $row)) . $details),
      array('data' => $row->cat, 'align' => 'right'),
      array('data' => $row->points, 'align' => 'right'),
  );
}
