$Id$

Copyright 2005 http://2bits.com

Description
-----------
This module provides the ability for users to gain points when they
do certain actions, such as:

- posting a node (different points can be awarded for different
  node types, e.g. page, story, forum, image, ...etc.)
- posting a comment
- moderating a comment

Additionally, points can be awarded for actions done by other modules,
including:
- voting on a node (requires the nodevote module)
- referring a person to the site (requires referral module)
- a visitor comes to the site via clicking on an affiliate link
  (requires the affiliates module)
- voting up or down a node (requires the vote_up_down module)
- inviting a person to register on the site (requires invite module)
- invited person actually registers on the site
- purchasing from your e-commerce store (reward points)

Using real money, users can purchase points from your ecommerce store
as well.

Moreover, the points can be used as currency for ecommerce as well,
as in a form of payment

Upon deleting a node or a comment the number of points is subtracted.

The number of points for each of the above actions is configurable by
the site adminsitrator.

A block displays the number of points the user gained. Another block 
displays the top 5 users who earned points.

This module is useful in providing an incentive for users to participate
in the site, and be more active.

A transaction log is created for each event. The log is viewable by
the admin.

Points can be moderated, i.e. approval can be done by the admin at a later
time.

Initally sponsored by: http://artalyst.com

Extended Version
----------------
A commercial extended version of this module allows users to join new
roles with more permissions as they gain points.

Contact the author for details.

Installation
------------
To install this module, do the following:

1. Extract the tar ball that you downloaded from Drupal.org.

2. Upload the userpoints directory and all its contents to your
   modules directory.

Configuration
-------------
To enable this module do the following:

1. Go to Admin -> Modules, and enable userpoints.
   Check the messages to make sure that you did not get any errors
   on database creation.

2. Go to Admin -> Settings -> userpoints.

   Configure the options as per your requirements

3. Go to Admin -> Access Control and enable viewing for the roles you want.

For configuring with e-commerce, you have to have the ecommerce modules
installed and configured.

- User points can be used as a form of payment, with an admin defined
  multiplier

- Users gain points when purchasing items via e-commerce for every dollar
  they spend.

This is useful as a reward system.

This also allows purchasing of points for real money. You have to setup
a non-shippable product, and adjust the multiplier accordingly.

API
---
This modules provides an application programming interface (API), which is
callable and actionable by other modules.

The functions are:

userpoints_userpointsapi('points', $points, $uid, $event, $description)
  Use this function to award points to a user.

  The arguments are:

  $op
    Must be 'points'.

  $points
    number of points to add (if positive) or subtract (if negative)

  $uid
    user ID to award points to.

  $event
    an identified of the event the points is being awarded for, this
    is a short word, and will be recorded in the transaction log.

  $description
    a description of the event. This is optional and is a more verbose
    version of the event identifier.

hook_userpoints($op, $points, $uid, $event) 

  Use this hook to act upon certain operations. When other modules award
  points to a user, your hook will be called, among others.

  The arguments are:

  $op: The operation to be acted upon.
    'setting'
      Pass a field set and fields that would be displayed in the userpoints
      settings page. For example, this can be used for your program to ask
      the admin to set a number of points for certain actions your module
      performs. The function should return an array object conforming to
      FormsAPI structure.

    'points before'
      Calls your module, and others, before the points are processed. You can
      prevent points from being awarded by returning FALSE.

    'points after'
      Calls your module, and others, after points are processed. You can take
      certain actions if you so wish. Return value is ignored.

   The rest of the arguments are the same as the userpoints_userpointsapi()
   function.
 
$points = userpoints_get_current_points($uid) 
   You can call this function to know how much points a use has.
   return value is the number of points.

Bugs/Features/Patches:
----------------------
If you want to report bugs, feature requests, or submit a patch, please do so
at the project page on the Drupal web site.
http://drupal.org/project/userpoints

Author
------
Khalid Baheyeldin (http://baheyeldin.com/khalid and http://2bits.com)

If you use this module, find it useful, and want to send the author
a thank you note, then use the Feedback/Contact page at the URL above.

The author can also be contacted for paid customizations of this
and other modules.
