====================
Developer guidelines
====================

These guidelines are meant to help you develop code for this package: documentation, tests, bug fixes, features, etc.


General rules
=============
#. Binary files can NOT be added to the repository, ideally you should develop your code in `.py` files.
   Example of files that can NOT be added to the repository: :code:`.png, .tif, .npy, .pkl, .dat, .html`
   If you have doubts about the files you are committing, you can always ask for help.
#. Branch naming follows the pattern :code:`<#>-feature_description` where :code:`<#>` represents the issue number.
#. You need to have implemented tests for your code before merging into develop.
#. You need to run all the tests before opening a merge request.

Code style
==========
Coding style is enforced with pre-commit hooks, you should install them by doing the following in the root directory:

.. code-block::

    sudo apt install pre-commit
    pre-commit install

Tests
=====
As mentioned before, your code should ideally be tested thoroughly. For this we use GoogleTests. You can find
examples of it in the already created unittests in our repository.
To run all tests you can run :code:`make test` on the build directory.

.. _contributing-a-feature:

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
hosted in online: `XiLens documentation <https://imsy.pages.dkfz.de/issi/xilens/>`_.

Contributing a feature/bug fix
==============================
If you have doubts on how to finish your feature branch, you can always ask for help

#. Create issue on `GitHub <https://git.dkfz.de/imsy/issi/xilens/-/issues>`_, if a task does not exist yet.
#. Assign the task to you.
#. Create a fork of the repository.
#. Create a new branch.

   .. tip::

      The `branch name` has to match the following pattern: `<issue-number>-<short_description_of_task>`

#. Implement your feature
#. Update :code:`feature` branch: :code:`git checkout <branch_name> && git merge develop`.
#. Run tests in :code:`feature` branch
#. Create a merge request for your feature.
   select `develop` as the destination branch.
#. The branch will be reviewed and automatically merged if there are no requested changes.

Finished or deprecated branches
===============================
Branches that are no longer in use or that have been merged into develop should be deleted.
If you merged into develop but your branch is still there, you can delete it manually by doing the following.

.. tip::

    Make sure that your code is already in origin/develop (before deleting branches) .

.. code-block:: bash

    git branch -d <branch-name>
    git push origin --delete <branch-name>
