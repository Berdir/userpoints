<?php

interface UserpointsTransactionInterface
{

  function setEntity($entity_type, $entity_id);

  function setReference($reference);

  function setDescription($description);

  function setStatus($status);

  function setExpirationDate($expirydate);

  function setExpired($expired);

  function setUid($uid);

  function setApproverUid($uid);

  function setPoints($points);

  function setTimestamp($timestamp);

  function setParent($txn_id);

  function setTid($tid);

  function setOperation($operation);

  function setDisplay($display);

  function getEntity();

  function getReference();

  function getDescription();

  function getStatus();

  function getExpirationDate();

  function getExpired();

  function getUid();

  function getApproverUid();

  function getPoints();

  function getTimestamp();

  function getChanged();

  function getParent();

  function getTid();

  function getOperation();

  function getDisplay();

  function save();

  function deny($reason);

  function isDenied();

  function getDenyReasons();
}

/**
 * A Userpoints transaction.
 */
class UserpointsTransaction
{

  protected $txn_id;
  protected $uid;
  protected $points;
  protected $approver_uid;
  protected $description;
  protected $status;
  protected $reference;
  protected $expired;
  protected $expirydate;
  protected $time_stamp;
  protected $changed;
  protected $parent_txn_id = 0;
  protected $tid;
  protected $operation;
  protected $entity_type;
  protected $entity_id = 0;
  protected $denied_reasons = array();
  protected $display = TRUE;
  protected $message;

  /**
   * Start a new transaction or update an existing one.
   *
   * @param $txn_id
   *   Transaction id if an existing transaction should be loaded.
   */
  function __construct($txn_id = NULL) {
    if ($txn_id) {
      $txn = userpoints_transaction_load($txn_id);
      if (!$txn) {
        throw new UserpointsTransactionNotFoundException($txn_id);
      }
      // @todo Improve this.
      foreach ($txn as $property => $value) {
        $this->$property = $value;
      }
    }
    else {
      // Set default values.
      $this->status = variable_get('userpoints_points_moderation', USERPOINTS_TXN_STATUS_APPROVED);
      $this->tid = userpoints_get_default_tid();
      $this->expirydate = userpoints_get_default_expiry_date();
      $this->time_stamp = REQUEST_TIME;
    }
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
    $this->entity_type = $entity_type;
    $this->entity_id = $entity_id;
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

    $this->reference = $reference;
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

    $this->description = $description;
    return $this;
  }

  /**
   * Set the status for a transaction.
   *
   * You can use the approve(), decline() and pending() functions as a shortcut.
   *
   * @param $status
   *   One of the following constants: USERPOINTS_TXN_STATUS_APPROVED,
   *   USERPOINTS_TXN_STATUS_DECLINED, USERPOINTS_TXN_STATUS_PENDING.
   *
   * @return UserpointsTransaction
   */
  function setStatus($status) {
    $this->checkChange();

    // Check allowed values.
    if (!in_array($status, array(USERPOINTS_TXN_STATUS_APPROVED, USERPOINTS_TXN_STATUS_DECLINED, USERPOINTS_TXN_STATUS_PENDING))) {
      throw new UserpointsChangeException(t('Invalid status'));
    }

    $this->status = $status;
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

    if (!($expirydate > 0)) {
      throw new UserpointsInvalidArgumentException();
    }

    if ($expirydate < REQUEST_TIME) {
      $this->setExpired(TRUE);
    }

    $this->expirydate = $expirydate;
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
    $this->expired = $expired;
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
    $this->uid = $uid;
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
    $this->approver_uid = $uid;
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
      throw new UserpointsInvalidArgumentException();
    }

    $this->points = $points;
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
      return;
    }
    $this->time_stamp = $time_stamp;
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

    $this->parent_txn_id = $txn_id;
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
    $this->tid = $tid;
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

    $this->operation = $operation;
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
    if (!empty($this->entity_id) && !empty($this->entity_type)) {
      return array_shift(entity_load($this->entity_type, array($this->entity_id)));
    }
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
    return $this->reference;
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
    return $this->description;
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
    return $this->status;
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
    return $this->expirydate;
  }

  /**
   * Returns if the transaction is expired or not.
   *
   * @return
   *   TRUE if the transaction is expired, FALSE if not.
   *
   * @see UserpointsTransaction::setExpired()
   */
  function getExpired() {
    return $this->expired;
  }

  function getUid() {
    return $this->uid;
  }

  function getUser() {
    return user_load($this->uid);
  }

  function getApproverUid() {
    return $this->approver_uid;
  }

  function getPoints() {
    return $this->points;
  }

  function getTimestamp() {
    return $this->time_stamp;
  }

  function getChanged() {
    return $this->changed;
  }

  function getParent() {
    if (!empty($this->parent_txn_id)) {
      return userpoints_transaction_load($this->parent_txn_id);
    }
  }

  function getTid() {
    return $this->tid;
  }

  function getOperation() {
    return $this->operation;
  }

  function getDisplay() {
    return $this->display;
  }

  function getCategory() {
    // Load categories.
    $categories = userpoints_get_categories();
    $transaction->category = isset($categories[$this->tid]) ? $categories[$this->tid] : $categories[userpoints_get_default_tid()];
    return $transaction;
  }

  function pending() {
    $this->setStatus(USERPOINTS_TXN_STATUS_PENDING);
  }

  function approve() {
    $this->setStatus(USERPOINTS_TXN_STATUS_APPROVED);
  }

  function decline() {
    $this->setStatus(USERPOINTS_TXN_STATUS_DECLINED);
  }

  function save() {
    if (empty($this->points) || empty($this->uid) || empty($this->operation)) {
      throw new UserpointsTransactionIncompleteException();
    }

    module_invoke_all('userpoints_transaction_before', $this);

    if ($this->isDenied()) {
      throw new UserpointsTransactionDeniedException($this->getDenyReasons());
    }

    $this->changed = REQUEST_TIME;

    $fields = array(
      'points' => $this->points,
      'operation' => $this->operation,
      'uid' => $this->uid,
      'time_stamp' => $this->time_stamp,
      'changed' => $this->changed,
      'description' => $this->description,
      'entity_type' => $this->entity_type,
      'entity_id' => $this->entity_id,
      'parent_txn_id' => $this->parent_txn_id,
      'reference' => $this->reference,
      'status' => $this->status,
      'tid' => $this->tid,
    );

    if (empty($this->txn_id)) {
      $this->txn_id = db_insert('userpoints_txn')
          ->fields($fields)
          ->execute();
    }
    else {
      db_update('userpoints_txn')
        ->condition('txn_id', $this->txn_id)
        ->fields($fields)
        ->execute();
    }

    // Update totals.
    if ($this->getStatus() == USERPOINTS_TXN_STATUS_APPROVED) {
      $this->updateTotals();
    }

    module_invoke_all('userpoints_transaction_after', $this);

    return $this;
  }

  protected function updateTotals() {
    $total = array(
      'last_update' => REQUEST_TIME,
      'points' => $this->points + userpoints_get_current_points($this->uid, $this->tid),
    );
    // Update the total max points if necessary.
    $max_points_total = userpoints_get_max_points($this->uid, $this->tid);
    if ($total['points'] > $max_points_total) {
      $total['max_points'] = $total['points'];
    }
    db_merge('userpoints')
      ->key(array(
        'uid' => $this->uid,
        'tid' => $this->tid,
      ))
      ->fields($total)
      ->execute();

    $total = array(
      'last_update' => REQUEST_TIME,
      'points' => $this->points + userpoints_get_current_points($this->uid, 'all'),
    );
    // Update the total max points if necessary.
    $max_points_total = userpoints_get_max_points($this->uid, 'all');
    if ($total['points'] > $max_points_total) {
      $total['max_points'] = $total['points'];
    }
    db_merge('userpoints_total')
      ->key(array(
        'uid' => $this->uid,
      ))
      ->fields($total)
      ->execute();
  }

  protected function checkChange($only_new = FALSE) {
    if (!empty($this->txn_id) && ($only_new || $this->status <> USERPOINTS_TXN_STATUS_PENDING)) {
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
  }

  function getMessage() {
    if (!empty($this->message)) {
      return $this->message;
    }
       // Prepare arguments. They are the same for all string combinations.
    $categories = userpoints_get_categories();
    $arguments = array_merge(userpoints_translation(), array(
      '!username' => theme('username', array('account' => $account)),
      '%total' => userpoints_get_current_points($params['uid'], $params['tid']),
      '%category' => isset($categories[$params['tid']]) ? $categories[$params['tid']] : $categories[0],
    ));

    $view_own_points = user_access('view own userpoints') || user_access('view userpoints') || user_access('administer userpoints');
    $view_all_points = user_access('view userpoints') || user_access('administer userpoints');

    if ($params['status'] == USERPOINTS_TXN_STATUS_DECLINED) {
      // Points have been declined.
      if ($account->uid == $user->uid && $view_own_points) {
        $message = format_plural($params['points'], 'You did not receive approval for @count !point in the %category category.', 'You did not receive approval for @count !points in the %category category.', $arguments);
      }
      elseif ($view_all_points) {
        $message = format_plural($params['points'], '!username did not receive approval for @count !point in the %category category.', '!username did not receive approval for @count !points in the %category category.', $arguments);
      }
    }
    elseif (isset($params['points']) && $params['points'] < 0) {
      if ($params['status'] == USERPOINTS_TXN_STATUS_PENDING) {
        if ($account->uid == $user->uid && $view_own_points) {
          // Directly address the user if he is loosing points.
          $message = format_plural(abs($params['points']), 'You just had a !point deducted, pending administrator approval.', 'You just had @count !points deducted, pending administrator approval.', $arguments);
        }
        elseif ($view_all_points) {
          // Only display message about other users if user has permission to view userpoints.
          $message = format_plural(abs($params['points']), '!username just had a !point deducted, pending administrator approval.', '!username just had @count !points deducted, pending administrator approval.', $arguments);
        }
      }
      else {
        if ($account->uid == $user->uid && $view_own_points) {
          $message = format_plural(abs($params['points']), 'You just had a !point deducted and now have %total !points in the %category category.', 'You just had @count !points deducted and now have %total !points in the %category category.', $arguments);
        }
        elseif ($view_all_points) {
          $message = format_plural(abs($params['points']), '!username just had a !point deducted and now has %total !points in the %category category.', '!username just had @count !points deducted and now has %total !points in the %category category.', $arguments);
        }
      }
    }
    elseif (!empty($params['points'])) {
      if ($params['status'] == USERPOINTS_TXN_STATUS_PENDING) {
        if ($account->uid == $user->uid && $view_own_points) {
          // Directly address the user if he is loosing points.
          $message = format_plural(abs($params['points']), 'You just earned a !point, pending administrator approval.', 'You just earned @count !points, pending administrator approval.', $arguments);
        }
        elseif ($view_all_points) {
          // Only display message about other users if user has permission to view userpoints.
          $message = format_plural(abs($params['points']), '!username just earned a !point, pending administrator approval.', '!username just earned @count !points, pending administrator approval.', $arguments);
        }
      }
      else {
        if ($account->uid == $user->uid && $view_own_points) {
          $message = format_plural(abs($params['points']), 'You just earned a !point and now have %total !points in the %category category.', 'You just earned @count !points and now have %total !points in the %category category.', $arguments);
        }
        elseif ($view_all_points) {
          $message = format_plural(abs($params['points']), '!username just earned a !point and now has %total !points in the %category category.', '!username just earned @count !points and now has %total !points in the %category category.', $arguments);
        }
      }
    }
  }
}

class UserpointsChangeException extends Exception
{

}