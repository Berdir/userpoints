<?php

/**
 * @file
 * Contains the UserpointsTransaction and related classes.
 */

/**
 * A Userpoints transaction.
 *
 * @ingroup userpoints_api
 */
class UserpointsTransaction {

  /**
   * The transaction has been approved.
   */
  const STATUS_APPROVED = 0;
  /**
   * The transaction is pending for approval.
   */
  const STATUS_PENDING = 1;

  /**
   * The transaction has been declined.
   */
  const STATUS_DECLINED = 2;

  /**
   * The transaction id (primary key) of this transaction.
   */
  protected $txn_id = NULL;

  /**
   * The actual fields of this transaction.
   *
   * Stored in this array to simplify the save/update process.
   */
  protected $fields = array();

  /**
   * Deny reaaons.
   */
  protected $denied_reasons = array();

  /**
   *
   */
  protected $display = TRUE;
  protected $message;
  protected $orig_status;

  protected $aborted = FALSE;

  /**
   * Start a new transaction or update an existing one.
   *
   * @param $txn_id
   *   Transaction id if an existing transaction should be loaded.
   */
  function __construct($values = FALSE) {
    if ($values) {

      // The txn_id is stored in a separate property.
      $this->txn_id = $values['txn_id'];
      unset($values['txn_id']);

      // All other values are stored in the fields property.
      $this->fields = $values;
      $this->orig_status = $this->getStatus();
    }
    else {
      // Initalize fields.
      $this->fields = array(
        'uid' => 0,
        'points' => NULL,
        'operation' => NULL,
        'status' => variable_get('userpoints_points_moderation', UserpointsTransaction::STATUS_APPROVED),
        'tid' => userpoints_get_default_tid(),
        'expirydate' => userpoints_get_default_expiry_date(),
        'expired' => 0,
        'time_stamp' => REQUEST_TIME,
        'changed' => REQUEST_TIME,
        'approver_uid' => 0,
        'description' => '',
        'reference' => '',
        'parent_txn_id' => 0,
        'entity_type' => NULL,
        'entity_id' => 0,
      );
    }
  }

  function abort() {
    $this->aborted = TRUE;
  }

  function isAborted() {
    return $this->aborted;
  }

  /**
   * Define the referenced entity.
   *
   * @param $entity_type
   *   Entity type that should be referenced.
   * @param $entity_id
   *   Id of the referenced entity.
   *
   * @return UserpointsTransaction
   */
  function setEntity($entity_type, $entity_id) {
    $this->checkChange();

    // Ignore empty values.
    if (empty($entity_type) || empty($entity_id)) {
      return $this;
    }

    $this->fields['entity_type'] = $entity_type;
    $this->fields['entity_id'] = $entity_id;
    return $this;
  }

  /**
   * Add a free reference text to this transaction.
   *
   * @param $reference
   *   A string that serves as an internal reference for this transaction.
   *
   * @return UserpointsTransaction
   */
  function setReference($reference) {
    $this->checkChange();

    $this->fields['reference'] = $reference;
    return $this;
  }

  /**
   * Add a description to this transaction.
   *
   * Note that modules should instead implement hook_userpoints_info() and
   * provide a description for their operations. If a description is present, it
   * will be displayed instead of a description provided through the mentioned
   * hook.
   *
   * @param $description
   *   A description for this transaction.
   *
   * @return UserpointsTransaction
   */
  function setDescription($description) {
    $this->checkChange();

    $this->fields['description'] = $description;
    return $this;
  }

  /**
   * Set the status for a transaction.
   *
   * You can use the approve(), decline() and pending() functions as a shortcut.
   *
   * @param $status
   *   One of the following constants: UserpointsTransaction::STATUS_APPROVED,
   *   UserpointsTransaction::STATUS_DECLINED, UserpointsTransaction::STATUS_PENDING.
   *
   * @return UserpointsTransaction
   */
  function setStatus($status) {
    $this->checkChange();

    // Check allowed values.
    if (!in_array($status, array(UserpointsTransaction::STATUS_APPROVED, UserpointsTransaction::STATUS_DECLINED, UserpointsTransaction::STATUS_PENDING))) {
      $this->abort();
      throw new UserpointsChangeException(t('Invalid status'));
    }

    // Preserve the original status to be able to check if changes in this
    // transaction are still allowed.
    $this->orig_status = $this->getStatus();

    $this->fields['status'] = $status;
    return $this;
  }

  /**
   * Set the expiration date of a transaction.
   *
   * Setting it to a date in the past will immediatly expire the transaction.
   *
   * @param $expirydate
   *   Timestamp of the expiration date.
   *
   * @return UserpointsTransaction
   */
  function setExpirationDate($expirydate) {
    $this->checkChange();

    if ($expirydate > 0 || $expirydate === 0 || $expirydate === NULL) {
      $this->fields['expirydate'] = (int)$expirydate;
    }
    else {
      $this->abort();
      throw new UserpointsInvalidArgumentException(t('Expiration date must be an integer'));
    }
    return $this;
  }

  /**
   * Marks a transaction as expired.
   *
   * This does not affect the points total, instead, a reverting transaction
   * must be created, see userpoints_expire_transactions().
   *
   * @param $expired
   *   TRUE if the transaction should be marked as expired, FALSE if not.
   *
   * @return UserpointsTransaction
   */
  function setExpired($expired) {

    // A transaction can always be expired but this can not be reversed.
    if (!$expired && $this->fields['expired']) {
      $this->checkChange();
    }
    $this->fields['expired'] = $expired;
    return $this;
  }

  /**
   * The user id of the user to which this transaction belongs.
   *
   * @param $uid
   *   The user id.
   *
   * @return UserpointsTransaction
   */
  function setUid($uid) {
    $this->checkChange();
    $this->fields['uid'] = $uid;
    return $this;
  }

  /**
   * Set the user who approved this transaction.
   *
   * @param $uid
   *   The user id of the approver.
   *
   * @return UserpointsTransaction
   */
  function setApproverUid($uid) {
    $this->checkChange();
    $this->fields['approver_uid'] = (int)$uid;
    return $this;
  }

  /**
   * Define the points amount of this transaction, which can be any positive
   * or negative amount but not 0.
   *
   * @param $points
   *   The points as an integer.
   *
   * @return UserpointsTransaction
   */
  function setPoints($points) {
    $this->checkChange();

    // Empty points amount is not allowed.
    if (empty($points)) {
      $this->abort();
      throw new UserpointsInvalidArgumentException();
    }

    $this->fields['points'] = $points;
    return $this;
  }

  /**
   * Set the creation date of this transaction.
   *
   * This can only be set if the userpoints_transaction_timestamp variable is
   * set to false. If that is set to true, the current timestamp is always
   * enforced.
   *
   * @param $time_stamp
   *   The timestamp of the transaction.
   *
   * @return UserpointsTransaction
   */
  function setTimestamp($time_stamp) {
    $this->checkChange(TRUE);

    if (variable_get('userpoints_transaction_timestamp', 1)) {
      return $this;
    }
    $this->fields['time_stamp'] = $time_stamp;
    return $this;
  }

  /**
   * Define a parent transaction for this.
   *
   * For example, when expiring another transaction, this allows to add a
   * reference to the expired transaction.
   *
   * @param $txn_id
   *   The transaction id of the referenced transaction.
   *
   * @return UserpointsTransaction
   */
  function setParent($txn_id) {
    $this->checkChange();

    $this->fields['parent_txn_id'] = $txn_id;
    return $this;
  }

  /**
   * Set the category (term tid) of this transaction.
   *
   * @param $tid
   *   The tid, a term id.
   *
   * @return UserpointsTransaction
   */
  function setTid($tid) {
    $this->checkChange();
    $this->fields['tid'] = $tid;
    return $this;
  }

  /**
   * Set the operation string for this transaction.
   *
   * A string that can identify this transaction. Can be used to provide a
   * custom, translatable, optionally dynamic reason for this transaction in
   * transaction listings. See hook_userpoints_info().
   *
   * This typically indicates the reason for this transaction, e.g. the user
   * commented, voted, logged in etc.
   *
   * This should be understood as a machine name, e.g. mymodule_category_action.
   *
   * @param $operation
   *   A string to identify this type of transaction.
   *
   * @return UserpointsTransaction
   */
  function setOperation($operation) {
    $this->checkChange();

    $this->fields['operation'] = $operation;
    return $this;
  }

  /**
   * Define if a message should be displayed to the user about this transaction.
   *
   * This can also be overriden by the userpoints_display_message setting. If
   * that setting is disabled, messages are never displayed.
   *
   * @param $display
   *   TRUE if a message should be displayed, FALSE if not. Defaults to TRUE.
   *
   * @return UserpointsTransaction
   */
  function setDisplay($display) {
    $this->display = $display;
    return $this;
  }

  /**
   * Get the referenced entity, if any.
   *
   * @return
   *   An entity object or NULL.
   */
  function getEntity() {
    if (!empty($this->fields['entity_id']) && !empty($this->fields['entity_type']) && entity_get_info($this->fields['entity_type'])) {
      return array_shift(entity_load($this->fields['entity_type'], array($this->fields['entity_id'])));
    }
  }

  /**
   * Get the referenced entity type, if any.
   *
   * @return
   *   The entity type as a string.
   */
  function getEntityType() {
    return $this->fields['entity_type'];
  }

  /**
   * Get the referenced entity id, if any.
   *
   * @return
   *   The entity id as an integer.
   */
  function getEntityId() {
    return $this->fields['entity_id'];
  }

  /**
   * The reference string of this transaction, if defined.
   *
   * @return
   *   A reference string or NULL.
   *
   * @see UserpointsTransaction::setReference()
   */
  function getReference() {
    return $this->fields['reference'];
  }

  /**
   * The description string of this transaction, if defined.
   *
   * @return
   *   A description string or NULL.
   *
   * @see UserpointsTransaction::setDescription()
   */
  function getDescription() {
    return $this->fields['description'];
  }

  /**
   * The status of this transaction.
   *
   * @return
   *   The status of this transaction (approved, declined, pending).
   *
   * @see UserpointsTransaction::setStatus()
   */
  function getStatus() {
    return $this->fields['status'];
  }

  /**
   * The expiration date of this transaction, if defined.
   *
   * @return
   *   The expiration date as timestamp or NULL.
   *
   * @see UserpointsTransaction::setExpirationDate()
   */
  function getExpirationDate() {
    return $this->fields['expirydate'];
  }

  /**
   * Returns if the transaction is expired or not.
   *
   * @return
   *   TRUE if the transaction is expired, FALSE if not.
   *
   * @see UserpointsTransaction::setExpired()
   */
  function isExpired() {
    return $this->fields['expired'];
  }

  function getUid() {
    return $this->fields['uid'];
  }

  function getUser() {
    return user_load($this->fields['uid']);
  }

  function getApproverUid() {
    return $this->fields['approver_uid'];
  }

  function getApprover() {
    return user_load($this->fields['approver_uid']);
  }

  function getPoints() {
    return $this->fields['points'];
  }

  function getTimestamp() {
    return $this->fields['time_stamp'];
  }

  function getChanged() {
    return $this->fields['changed'];
  }

  function getParent() {
    if (!empty($this->fields['parent_txn_id'])) {
      return userpoints_transaction_load($this->fields['parent_txn_id']);
    }
  }

  function getTid() {
    return $this->fields['tid'];
  }

  function getOperation() {
    return $this->fields['operation'];
  }

  function getTxnId() {
    return $this->txn_id;
  }

  function getDisplay() {
    return $this->display;
  }

  function getCategory() {
    // Load categories.
    $categories = userpoints_get_categories();
    return isset($categories[$this->getTid()]) ? $categories[$this->getTid()] : $categories[userpoints_get_default_tid()];
  }

  function pending() {
    $this->setStatus(UserpointsTransaction::STATUS_PENDING);
    return $this;
  }

  function approve() {
    $this->setStatus(UserpointsTransaction::STATUS_APPROVED);
    return $this;
  }

  function decline() {
    $this->setStatus(UserpointsTransaction::STATUS_DECLINED);
    return $this;
  }

  function isPending() {
    return $this->getStatus() == UserpointsTransaction::STATUS_PENDING;
  }

  function isDeclined() {
    return $this->getStatus() == UserpointsTransaction::STATUS_DECLINED;
  }

  function isApproved() {
    return $this->getStatus() == UserpointsTransaction::STATUS_APPROVED;
  }

  function save() {
    if (!$this->getPoints() || !$this->getUid() || !$this->getOperation()) {
      $this->abort();
      throw new UserpointsTransactionIncompleteException();
    }

    module_invoke_all('userpoints_transaction_before', $this);

    if ($this->isDenied()) {
      $this->abort();
      throw new UserpointsTransactionDeniedException($this->getDenyReasons());
    }

    if (empty($this->txn_id)) {
      $this->txn_id = db_insert('userpoints_txn')
          ->fields($this->fields)
          ->execute();
    }
    else {
      db_update('userpoints_txn')
        ->condition('txn_id', $this->txn_id)
        ->fields($this->fields)
        ->execute();
    }

    // Update totals if the transaction is approved and not expired.
    if ($this->isApproved() && !$this->isExpired()) {
      $this->updateTotals();
    }
    module_invoke_all('userpoints_transaction_after', $this);

    // Display a message unless disabled or no message exists.
    if ($this->getDisplay() && $message = $this->getMessage()) {
      drupal_set_message($message);
    }

    // Reset original status to current one.
    $this->orig_status = $this->getStatus();
    return $this;
  }

  protected function updateTotals() {
    $this->updateTotalsCategory($this->getTid());
    $this->updateTotalsCategory('all');
  }

  protected function updateTotalsCategory($tid) {
    $table = 'userpoints';
    if ($tid === 'all') {
      $table = 'userpoints_total';
    }

    $total = array(
      'last_update' => REQUEST_TIME,
      'points' => $this->getPoints() + userpoints_get_current_points($this->getUid(), $tid),
    );
    // Update the total max points if necessary.
    $max_points_total = userpoints_get_max_points($this->getUid(), $tid);
    if ($total['points'] > $max_points_total) {
      $total['max_points'] = $total['points'];
    }

    $keys = array(
      'uid' => $this->getUid(),
    );
    if ($tid !== 'all') {
      $keys['tid'] = $tid;
    }

    db_merge($table)
      ->key($keys)
      ->fields($total)
      ->execute();
  }

  protected function checkChange($only_new = FALSE) {
    if (!empty($this->txn_id) && ($only_new || $this->orig_status <> UserpointsTransaction::STATUS_PENDING)) {
      $this->abort();
      throw new UserpointsChangeException(t('This transaction is saved and approved or pending and can not be changed.'));
    }
  }

  function deny($reason) {
    $this->denied_reasons[] = $reason;
  }

  function isDenied() {
    return!empty($this->denied_reasons);
  }

  function getDenyReasons() {
    return $this->denied_reasons;
  }

  function setMessage($message) {
    $this->message = $message;
    return $this;
  }

  function getMessage() {
    global $user;

    if (!empty($this->message)) {
      return $this->message;
    }
    // Prepare arguments. They are the same for all string combinations.
    $categories = userpoints_get_categories();
    $arguments = array_merge(userpoints_translation(), array(
        '!username' => theme('username', array('account' => $this->getUser())),
        '%total' => userpoints_get_current_points($this->getUid(), $this->getTid()),
        '%category' => $this->getCategory(),
      ));

    $view_own_points = user_access('view own userpoints') || user_access('view userpoints') || user_access('administer userpoints');
    $view_all_points = user_access('view userpoints') || user_access('administer userpoints');

    $message = NULL;
    if ($this->isDeclined()) {
      // Points have been declined.
      if ($this->getUid() == $user->uid && $view_own_points) {
        $message = format_plural($this->getPoints(), 'You did not receive approval for @count !point in the %category category.', 'You did not receive approval for @count !points in the %category category.', $arguments);
      }
      elseif ($view_all_points) {
        $message = format_plural($this->getPoints(), '!username did not receive approval for @count !point in the %category category.', '!username did not receive approval for @count !points in the %category category.', $arguments);
      }
    }
    elseif ($this->getPoints() < 0) {
      if ($this->isPending()) {
        if ($this->getUid() == $user->uid && $view_own_points) {
          // Directly address the user if he is loosing points.
          $message = format_plural(abs($this->getPoints()), 'You just had a !point deducted, pending administrator approval.', 'You just had @count !points deducted, pending administrator approval.', $arguments);
        }
        elseif ($view_all_points) {
          // Only display message about other users if user has permission to view userpoints.
          $message = format_plural(abs($this->getPoints()), '!username just had a !point deducted, pending administrator approval.', '!username just had @count !points deducted, pending administrator approval.', $arguments);
        }
      }
      else {
        if ($this->getUid() == $user->uid && $view_own_points) {
          $message = format_plural(abs($this->getPoints()), 'You just had a !point deducted and now have %total !points in the %category category.', 'You just had @count !points deducted and now have %total !points in the %category category.', $arguments);
        }
        elseif ($view_all_points) {
          $message = format_plural(abs($this->getPoints()), '!username just had a !point deducted and now has %total !points in the %category category.', '!username just had @count !points deducted and now has %total !points in the %category category.', $arguments);
        }
      }
    }
    else {
      if ($this->isPending()) {
        if ($this->getUid() == $user->uid && $view_own_points) {
          // Directly address the user if he is loosing points.
          $message = format_plural(abs($this->getPoints()), 'You just earned a !point, pending administrator approval.', 'You just earned @count !points, pending administrator approval.', $arguments);
        }
        elseif ($view_all_points) {
          // Only display message about other users if user has permission to view userpoints.
          $message = format_plural(abs($this->getPoints()), '!username just earned a !point, pending administrator approval.', '!username just earned @count !points, pending administrator approval.', $arguments);
        }
      }
      else {
        if ($this->getUid() == $user->uid && $view_own_points) {
          $message = format_plural(abs($this->getPoints()), 'You just earned a !point and now have %total !points in the %category category.', 'You just earned @count !points and now have %total !points in the %category category.', $arguments);
        }
        elseif ($view_all_points) {
          $message = format_plural(abs($this->getPoints()), '!username just earned a !point and now has %total !points in the %category category.', '!username just earned @count !points and now has %total !points in the %category category.', $arguments);
        }
      }
    }
    return $message;
  }

  function getOperationInfo() {
    return userpoints_get_info($this->getOperation());
  }

  /**
   * Returns a descriptive reason for this transaction.
   *
   * The following resources are considered, in this order:
   *
   *  * description key in the information array for that operation.
   *  * description of the transaction.
   *  * name of the operation.
   *
   * @param $options
   *   Array of options:
   *   - link: If FALSE, no link is generated to the linked entity even if there
   *     were one. Defaults to TRUE.
   *   - truncate: Define if the reason should be truncated. Defaults to TRUE.
   *   - skip_description: Allows to skip the eventually existing custom
   *     description a transaction has and always uses the generated description.
   *
   * @return
   *   The reason for that transaction, linked to the referenced entity if
   *   available.
   */
  function getReason(array $options = array()) {

    // Default options.
    $options += array(
      'link' => TRUE,
      'truncate' => TRUE,
    );

    $safe = FALSE;
    // Check transaction description first to allow custom overrides.
    if (empty($options['skip_description']) && $description = $this->getDescription()) {
      $reason = $description;
    }
    else {
      $info = $this->getOperationInfo();
      // Check if there is a valid description callback defined for this
      // operation.
      if (!empty($info['description callback']) && function_exists($info['description callback'])) {
        $reason = $info['description callback']($this, $this->getEntity());
        $safe = TRUE;
      }
      // Try static description key.
      elseif (!empty($info['description'])) {
        $reason = $info['description'];
        $safe = TRUE;
      }
    }
    // Fallback to the operation name if there is no source.
    if (empty($reason)) {
      $reason = $this->getOperation();
    }

    // Truncate description.
    $attributes = array();
    $stripped_reason = strip_tags($reason);
    if ($options['truncate'] && drupal_strlen($stripped_reason) > variable_get('userpoints_truncate', 30) + 3) {
      // The title attribute will be check_plain()'d again drupal_attributes(),
      // avoid double escaping.
      $attributes['title'] = html_entity_decode($stripped_reason, ENT_QUOTES);
      $reason = truncate_utf8($stripped_reason, variable_get('userpoints_truncate', 30), FALSE, TRUE);
    }

    // Link to the referenced entity, if available.
    if ($this->getEntity() && $options['link']) {
      $uri = entity_uri($this->getEntityType(), $this->getEntity());
      if ($uri) {
        $reason = l($reason, $uri['path'], $uri['options'] + array('html' => $safe, 'attributes' => $attributes));
      }
    }
    if ((!$this->getEntity() || empty($uri)) && !$safe) {
      // Escape possible user provided reason.
      $reason = check_plain($reason);
    }
    return $reason;
  }

  /**
   * Returns a list of operations as links.
   *
   * @param $show_view
   *   FALSE if the view link should not be displayed. Defaults to TRUE.
   *
   * @return
   *   A string with operation links.
   */
  function getActions($show_view = TRUE) {
    $actions = array();
    $url_options = array('query' => drupal_get_destination());

    $uri = userpoints_transaction_uri_callback($this, TRUE);
    $url_prefix = $uri['path'];

    if ($show_view && userpoints_access_view_transaction($this)) {
      $actions[] = l('view', $url_prefix . '/view');
    }
    if (userpoints_admin_access('edit')) {
      $actions[] = l('edit', $url_prefix . '/edit', $url_options);
    }
    if (userpoints_admin_access('moderate') && $this->isPending()) {
      $actions[] = l('approve', $url_prefix . '/approve', $url_options);
      $actions[] = l('decline', $url_prefix . '/decline', $url_options);
    }
    return implode(' ', $actions);
  }

  /**
   * Returns a single row for a transaction listing.
   *
   * @param $settings
   *   Array with settings about which column shall be displayed. All settings
   *   default to TRUE.
   *   - show_category, show category column.
   *   - show_user, show user column.
   *   - show_status, show status column.
   *
   * @return
   *   A table row array for use with theme_table().
   */
  function getTableRow($settings = array()) {
    $settings += array(
      'show_user' => TRUE,
      'show_status' => TRUE,
    );

    $stati = userpoints_txn_status();
    $css_stati = array(
      UserpointsTransaction::STATUS_APPROVED => 'approved',
      UserpointsTransaction::STATUS_DECLINED => 'declined',
      UserpointsTransaction::STATUS_PENDING => 'pending',
    );
    $row = array('class' => array(
        'userpoints-transaction-row-status-' . $css_stati[$this->getStatus()],
        'userpoints-transaction-row-category-' . $this->getTid()),
    );
    if ($settings['show_user']) {
      $row['data'][] = array(
        'data' => theme('username', array('account' => $this->getUser())),
        'class' => array('userpoints-transactions-field-user'),
      );
    }
    $row['data'][] = array(
      'data' => $this->getPoints(),
      'class' => array('userpoints-transactions-field-points', 'userpoints-transaction-points-' . ($this->getPoints() > 0 ? 'positive' : 'negative')),
    );

    $categories = userpoints_get_categories();
    if (count($categories) > 1) {
      $row['data'][] = array(
        'data' => $this->getCategory(),
        'class' => array('userpoints-transactions-field-category'),
      );
    }
    $row['data'][] = array(
      'data' => format_date($this->getTimestamp(), 'small'),
      'class' => array('userpoints-transactions-field-timestamp'),
    );
    $row['data'][] = array(
      'data' => $this->getReason(),
      'class' => array('userpoints-transactions-field-reason'),
    );
    if ($settings['show_status']) {
      $row['data'][] = array(
        'data' => $stati[$this->getStatus()],
        'class' => array('userpoints-transactions-field-status'),
      );
    }
    $row['data'][] = array(
      'data' => $this->getActions(),
      'class' => array('userpoints-transactions-field-actions'),
    );
    return $row;
  }

  function __destruct() {
    // Automatically save new transactions to improve DX.
    if (!$this->getTxnId() && !$this->isAborted()) {
      $this->save();
    }
  }

  /**
   * Magic function to allow access to property by name.
   */
  function __get($name) {

    // Compatibility with dpm().
    if (strpos($name, 'krumo') !== FALSE) {
      if (!isset($this->$name)) {
        $this->$name = NULL;
      }
      return $this->$name;
    }

    $method = 'get' . str_replace('_', '', $name);
    if (method_exists($this, $method)) {
      return $this->$method();
    }
    else {
      throw new UserpointsInvalidPropertyException($name);
    }
  }

}

class UserpointsChangeException extends Exception {

}

class UserpointsInvalidPropertyException extends Exception {
  function __construct($name, $code = NULL, $previous = NULL) {
    parent::__construct(t('Userpoints transaction does not have a @property property.', array('@property' => $name)), $code, $previous);
  }
}

class UserpointsInvalidArgumentException extends Exception {

}

class UserpointsTransactionIncompleteException extends Exception {

}

