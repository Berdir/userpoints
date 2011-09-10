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
   * The transaction id (primary key) of this transaction
   *
   * @var int
   */
  protected $txn_id = NULL;

  /**
   * The actual fields of this transaction.
   *
   * Stored in this array to simplify the save/update process.
   *
   * @var array
   */
  protected $fields = array();

  /**
   * Deny reasons.
   *
   * @var array
   */
  protected $denied_reasons = array();

  /**
   * TRUE if the transaction should display a message.
   *
   * @var boolean
   */
  protected $display = TRUE;

  /**
   * Overriden message, is used instead of the default in
   * UserpointsTransaction::getReason() if existend.
   *
   * @var string
   */
  protected $message;

  /**
   * The original status of this transaction, used for denying changes to this
   * transaction if is is not pending anymore.
   *
   * @var int
   */
  protected $orig_status;

  /**
   * If a transaction is aborted, it will not be saved automatically.
   *
   * Automatically set whenever an exception occurs.
   */
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

  /**
   * Marks this transaction as aborted.
   */
  function abort() {
    $this->aborted = TRUE;
  }

  /**
   * Checks if this transaction is aborted.
   */
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
   *   UserpointsTransaction::STATUS_DECLINED,
   *   UserpointsTransaction::STATUS_PENDING.
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

  /**
   * Returns the UID of the user this transaction belongs to.
   *
   * @return
   *   The uid of the user.
   *
   * @see UserpointsTransaction::setUid()
   */
  function getUid() {
    return $this->fields['uid'];
  }

  /**
   * Returns the user object this transaction belongs to.
   *
   * @return
   *   loaded user object for the user this transaction belongs to.
   *
   * @see UserpointsTransaction::setUid()
   * @see UserpointsTransaction::getUid()
   */
  function getUser() {
    return user_load($this->fields['uid']);
  }

  /**
   * Returns the uid of the user who approved this transaction.
   *
   * @return
   *   The approver uid.
   *
   * @see UserpointsTransaction::setApproverUid()
   */
  function getApproverUid() {
    return $this->fields['approver_uid'];
  }

  /**
   * The loaded user object of the user who approved this transaction.
   *
   * @return
   *   User object.
   *
   * @see UserpointsTransaction::setApproverUid()
   * @see UserpointsTransaction::getApproverUid()
   */
  function getApprover() {
    return user_load($this->fields['approver_uid']);
  }

  /**
   * The amount of points of this transaction.
   *
   * @return
   *   Points as an integer.
   *
   * @see UserpointsTransaction::setPoints()
   */
  function getPoints() {
    return $this->fields['points'];
  }

  /**
   * The timestamp of when this transaction was created.
   *
   * @return
   *   Unix timestamp of the creation date.
   *
   * @see UserpointsTransaction::setTimestamp()
   */
  function getTimestamp() {
    return $this->fields['time_stamp'];
  }

  /**
   * The timestamp of when this transaction last changed.
   *
   * @return
   *   Unix timestamp of the changed date.
   */
  function getChanged() {
    return $this->fields['changed'];
  }

  /**
   * Returns the parent transaction if there is any.
   *
   * @return UserpointsTransaction
   *   A userpoints transaction or NULL.
   *
   * @see UserpointsTransaction::setParent()
   */
  function getParent() {
    if (!empty($this->fields['parent_txn_id'])) {
      return userpoints_transaction_load($this->fields['parent_txn_id']);
    }
  }

  /**
   * The category id (term id) this transaction belongs to.
   *
   * Use UserpointsTransaction::getCategory() to get the name of the category.
   *
   * @return
   *   Term Id of this transaction.
   *
   * @see UserpointsTransaction::setTid()
   * @see UserpointsTransaction::getCategory()
   */
  function getTid() {
    return $this->fields['tid'];
  }

  /**
   * The operation of this transaction.
   *
   * @return
   *   The operation string of this transaction.
   *
   * @see UserpointsTransaction::setOperation()
   */
  function getOperation() {
    return $this->fields['operation'];
  }

  /**
   * The transaction id of this transaction.
   *
   * @return
   *   The id of this transaction as an integer. NULL if this transaction has
   *   not yet been saved.
   */
  function getTxnId() {
    return $this->txn_id;
  }

  /**
   * Check if a message about this transaction should be displayed.
   *
   * @return
   *   TRUE if a message should be displayed, FALSE otherwise.
   *
   * @see UserpointsTransaction::setDisplay()
   */
  function getDisplay() {
    return $this->display;
  }

  /**
   * The category of this transaction.
   *
   * @return
   *   The name of the category as a string. Name of th default category the
   *   term of this category has been deleted.
   *
   * @see UserpointsTransaction::setTid()
   * @see UserpointsTransaction::getTid()
   */
  function getCategory() {
    // Load categories.
    $categories = userpoints_get_categories();
    return isset($categories[$this->getTid()]) ? $categories[$this->getTid()] : $categories[userpoints_get_default_tid()];
  }

  /**
   * Mark this transaction as pending.
   *
   * @see UserpointsTransaction::setStatus
   */
  function pending() {
    $this->setStatus(UserpointsTransaction::STATUS_PENDING);
    return $this;
  }

  /**
   * Mark this transaction as approved.
   *
   * @see UserpointsTransaction::setStatus
   */
  function approve() {
    $this->setStatus(UserpointsTransaction::STATUS_APPROVED);
    return $this;
  }

  /**
   * Mark this transaction as declined.
   *
   * @see UserpointsTransaction::setStatus
   */
  function decline() {
    $this->setStatus(UserpointsTransaction::STATUS_DECLINED);
    return $this;
  }

  /**
   * Check if this transaction is pending.
   *
   * @see UserpointsTransaction::getStatus
   */
  function isPending() {
    return $this->getStatus() == UserpointsTransaction::STATUS_PENDING;
  }

  /**
   * Check if this transaction is declined.
   *
   * @see UserpointsTransaction::getStatus
   */
  function isDeclined() {
    return $this->getStatus() == UserpointsTransaction::STATUS_DECLINED;
  }

  /**
   * Check if this transaction is approved.
   *
   * @see UserpointsTransaction::getStatus
   */
  function isApproved() {
    return $this->getStatus() == UserpointsTransaction::STATUS_APPROVED;
  }

  /**
   * Save or update this transaction.
   *
   * It is not permitted to update a approved or denied transaction except
   * marking it as expird. Any attemt to change a property of such a transaction
   * will result in an immediate exception.
   */
  function save() {
    // Prevent saving when any of the required properties are missing.
    if (!$this->getPoints() || !$this->getUid() || !$this->getOperation()) {
      $this->abort();
      throw new UserpointsTransactionIncompleteException();
    }

    // Call the before hook to allow modules to change and deny this.
    module_invoke_all('userpoints_transaction_before', $this);

    // Abort if the transaction has been denied.
    if ($this->isDenied()) {
      $this->abort();
      throw new UserpointsTransactionDeniedException($this->getDenyReasons());
    }

    if (empty($this->txn_id)) {
      // This is a new transaction, save.
      $this->txn_id = db_insert('userpoints_txn')
          ->fields($this->fields)
          ->execute();
    }
    else {
      // Existing transaction, update.
      db_update('userpoints_txn')
        ->condition('txn_id', $this->txn_id)
        ->fields($this->fields)
        ->execute();
    }

    // Update totals if the transaction is approved and not expired.
    if ($this->isApproved() && !$this->isExpired()) {
      $this->updateTotals();
    }

    // Call the after hook so that modules can react on this.
    module_invoke_all('userpoints_transaction_after', $this);

    // Display a message unless disabled or no message exists.
    if ($this->getDisplay() && $message = $this->getMessage()) {
      drupal_set_message($message);
    }

    // Reset original status to current one.
    $this->orig_status = $this->getStatus();
    return $this;
  }

  /**
   * Update the total aggregations of the corresponding user.
   */
  protected function updateTotals() {
    // Update this category.
    $this->updateTotalsCategory($this->getTid());
    // Update the total over all categories.
    $this->updateTotalsCategory('all');
  }

  /**
   * Update the totals of a specific category.
   *
   * Updating the total of all categories is supported by using 'all' for the
   * $tid.
   */
  protected function updateTotalsCategory($tid) {
    $table = 'userpoints';
    if ($tid === 'all') {
      // Use a different table for the overall total.
      $table = 'userpoints_total';
    }

    // Always update the time stamp and the total points.
    $total = array(
      'last_update' => REQUEST_TIME,
      'points' => $this->getPoints() + userpoints_get_current_points($this->getUid(), $tid),
    );
    // Update the total max points if necessary.
    $max_points_total = userpoints_get_max_points($this->getUid(), $tid);
    if ($total['points'] > $max_points_total) {
      $total['max_points'] = $total['points'];
    }

    // The keys for the merge query. The tid is only added when not 'all'.
    $keys = array(
      'uid' => $this->getUid(),
    );
    if ($tid !== 'all') {
      $keys['tid'] = $tid;
    }

    // Save the updates.
    db_merge($table)
      ->key($keys)
      ->fields($total)
      ->execute();
  }

  /**
   * Checks if a change is allowed.
   *
   * Once a transaction is saved and either approved or declined, no alterations
   * of the data is allowed except marking it as expired.
   *
   * @param $only_new
   *   If TRUE, only allows changes if this transaction is new. Defaults to
   *   FALSE.
   */
  protected function checkChange($only_new = FALSE) {
    if (!empty($this->txn_id) && ($only_new || $this->orig_status <> UserpointsTransaction::STATUS_PENDING)) {
      $this->abort();
      throw new UserpointsChangeException(t('This transaction is saved and approved or pending and can not be changed.'));
    }
  }

  /**
   * Deny this transaction from being saved.
   *
   * This is typically called in hook_userpoints_transaction_before().
   *
   * @see UserpointsTransaction::isDenied()
   * @see UserpointsTransaction::getDenyReasons()
   */
  function deny($reason) {
    $this->denied_reasons[] = $reason;
  }

  /**
   * Check if this transaction is denied.
   *
   * A transaction is denied if there are any deny reasons.
   *
   * @see UserpointsTransaction::deny().
   * @see UserpointsTransaction::getDenyReasons()
   */
  function isDenied() {
    return!empty($this->denied_reasons);
  }

  /**
   * Returns the deny reasons for this transaction.
   *
   * @return
   *   An array with the reasons why this transaction was denied.
   *
   * @see UserpointsTransaction::deny()
   * @see UserpointsTransaction::isDenied()
   * @see UserpointsTransaction::getDenyReasons()
   */
  function getDenyReasons() {
    return $this->denied_reasons;
  }

  /**
   * Override the generated default message of this transaction.
   *
   * @param $message
   *   The message that should be displayed if configured to do so.
   *
   * @return UserpointsTransaction
   *
   * @see UserpointsTransaction::getMessage()
   * @see UserpointsTransaction::setDisplay()
   */
  function setMessage($message) {
    $this->message = $message;
    return $this;
  }

  /**
   * A message that can be displayed to the current user.
   *
   * If set, the message defined by UserpointsTransaction::setMessage() is used.
   * Otherwise, a message is displayed that takes into account the points amount
   * (negative or positive), the category, the status and if the transaction is
   * for the currently logged in user or not.
   *
   * @return
   *   A message string that describes this transaction to the currently logged
   *   in user. Can be empty if not automated message could have been generated.
   *
   * @see UserpointsTransaction::setMessage().
   */
  function getMessage() {
    global $user;

    // If set, use the overriden message.
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

  /**
   * Returns additional information about the operation of this transaction.
   *
   * @return
   *   Information about this operation as an array.
   *
   * @see userpoints_get_info()
   */
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
   *     description a transaction has and always use the generated description.
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

  /**
   * Automatically called if this object is goes out of scope and is destructed.
   */
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

/**
 * This exception is thrown when a property is changed after a saved transaction
 * has been approved or declined.
 */
class UserpointsChangeException extends Exception {

}

/**
 * This exception is thrown when trying to access an unknown property through
 * the magic UserpointsTransaction::__get() method.
 */
class UserpointsInvalidPropertyException extends Exception {
  function __construct($name, $code = NULL, $previous = NULL) {
    parent::__construct(t('Userpoints transaction does not have a @property property.', array('@property' => $name)), $code, $previous);
  }
}

/**
 * Thrown when trying to set a property to an invalid value.
 */
class UserpointsInvalidArgumentException extends Exception {

}

/**
 * Thrown when trying to save a transaction without points, uid or operation.
 */
class UserpointsTransactionIncompleteException extends Exception {

}
