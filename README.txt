$Id$

Copyright 2005 http://2bits.com

Description
-----------
This module provides the ability for users to gain points when they
do certain actions, such as:

- posting an image
- posting a page
- posting a story
- posting a forum topic
- posting a comment
- voting on a node (requires the nodevote module)

Upon deleting a node or a comment the number of points is subtracted.

The number of points for each of the above actions is configurable by
the site adminsitrator.

A block displays the number of points the user gained. Another block 
displays the top 5 users who earned points.

This module is useful in providing an incentive for users to participate
in the site, and be more active.

Sponsored by: http://artalyst.com

Extended Version
----------------
A commercial extended version of this module allows users to join new
roles with more permissions as they gain points.

Contact the author for details.

Installation
------------
To install this module, do the following:

1. Create the database table by pasting the included userpoints.mysql script
   in phpmyadmin or some other SQL tool you use. If you are familiar with
   mysql command line, you may do:

     mysql -udatabaseusername -ppassword databasename < userpoints.mysql

2. Upload or copy the userpoints.module file to your modules directory.

Configuration
-------------
To enable this module do the following:

1. Go to Admin -> Modules, and enable userpoints.

2. Go to Admin -> Settings -> userpoints.

   Assign how many points you want assigned for each action.

3. Go to Admin -> Access Control and enable viewing for the roles you want.

Bugs/Features/Patches:
----------------------
If you want to report bugs, feature requests, or submit a patch, please do so
at the project page on the Drupal web site.
http://drupal.org/project/nodevote

Author
------
Khalid Baheyeldin (http://baheyeldin.com/khalid and http://2bits.com)

If you use this module, find it useful, and want to send the author
a thank you note, then use the Feedback/Contact page at the URL above.

The author can also be contacted for paid customizations of this
and other modules.
