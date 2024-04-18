====================
Developer guidelines
====================

These guidelines are meant to help you develop code for this package: documentation, unittesting, bug fixes, features, releases, etc.


General rules
=============
#. Binary files can NOT be added to the repository, ideally you should develop your code in `.py` files.
   Example of files that can NOT be added to the repository: :code:`.png, .tif, .npy, .pkl, .dat, .html`
   If you have doubts about the files you are committing, you can always ask for help.
#. Branch naming follows `git flow` conventions, you can find a cheatsheet with the most common commands here
   `GitFlow <https://danielkummer.github.io/git-flow-cheatsheet/>`_.
   In order to initialize git flow in your environment you need to run:

   .. code-block:: bash

      git flow init -d

   Before starting your :code:`feature`, :code:`hotfix`, etc.
   you need to create a task in the `SUSICAM work board <https://git.dkfz.de/imsy/issi/susicam/-/issues>`_.
   After creating the task you will see a number like this `T18` in the title of the task. You should use this for
   naming your branch. For example:

   .. code-block:: bash

      git flow feature start T18-my_branch_name

   This will create a branch with the name :code:`feature/T18_my_branch_name`, notice that `git flow` prepends :code:`feature` to
   the branch name automatically. More details in :ref:`contributing-a-feature`.
#. You need to have implemented tests for your code before merging into develop.
#. You need to run all the tests before merging into develop.
#. If you need some real images in order
   to test your code, you can get several examples from :code:`skimage.data`, this is the preferred method.

Documentation
=============
Ideally all your code should be documented, the markup used for the documentation is `Doxygen-style <https://www
.doxygen.nl/manual/docblocks.html>`_. You should stick to it.
To build the documentation locally and check that yours is rendered properly you need to follow the next steps.

.. code-block:: bash

    cd doc
    sudo apt install -y doxygen graphviz
    pip install -U sphinx furo breathe
    doxygen Doxyfile
    make html SPHINXOPTS="-j4"


Now you can navigate to doc/build and open `index.html` to access the documentation. The documentation corresponding to the develop branch is also
hosted in online: `SUSICAM documentation <https://imsy.pages.dkfz.de/issi/susicam/>`_.

Unittesting
===============
As mentioned before, your code should ideally be tested thoroughly. For this we use GoogleTests. You can find
examples of it in the already created unittests in our repository.

.. _contributing-a-feature:

Contributing a feature/bug fix
==============================
If you have doubts on how to finish your feature branch, you can always ask for help

#. Create issue on the `SUSICAM work board <https://git.dkfz.de/imsy/issi/susicam/-/issues>`_, if a task does not
exist yet.
#. Assign the task to you.
#. Create the :code:`feature` branch: :code:`git flow feature start <branch_name>`.

   .. tip::

      `<branch_name>` has to match the following pattern: `T<task_number>-<short_description_of_task>`

#. Implement your code
#. Update :code:`feature` branch: :code:`git checkout <branch_name> && git merge develop`.
#. Run tests in :code:`feature` branch
#. Create a merge request for your feature in `GitLab merge request list <https://git.dkfz
   .de/imsy/issi/susicam/-/merge_requests>`_,
   select `develop` as the destination branch and assign reviewers.
#. The branch will be reviewed and automatically merged if there are no requested changes.

Preparing a release
===================
The version tag has to match the following pattern: :code:`v<x>.<y>.<z>` with :code:`x=major, y=minor, z=patch` version
number

#. Create a :code:`release` branch: :code:`git flow release start <version_tag>`.
#. Perform testing on the :code:`release` branch.
    * Run tests
    * Fix issues on the :code:`release` branch
    * Repeat testing procedure until no errors occur
#. Update release number. Following files have to be touched:
    * `doc/conf.py`
#. Finish a release: :code:`git flow release finish <branch_name> -T <version_tag> -m "Release of package"`
#. Push :code:`develop` branch and :code:`master` branch.
#. Push tags with :code:`git push origin <version_tag>`, or alternatively :Code:`git push --tags`
#. Change status of all release-relevant tasks to resolved (if not done already).


Finished or deprecated branches
------------------------------------
Branches that are no longer in use or that have been merged into develop should be deleted.
If you followed the guidelines for finishing a feature, your feature branch should have been deleted automatically.
If you merged into develop but your branch is still there, you can delete it manually by doing the following.

.. tip::

    Make sure that your code is already in develop (before delete branches) by going to `SUSICAM <https://git.dkfz
    .de/imsy/issi/susicam>`_ and checking the develop branch

.. code-block:: bash

    git branch -d <branch-name>
    git push origin --delete <branch-name>
