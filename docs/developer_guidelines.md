# Developer guidelines

These guidelines are meant to help you develop code for this package: documentation, tests, bug fixes, features, etc.


## General rules
1. Binary files can NOT be added to the repository, ideally you should develop your code in `.py` files.
   Example of files that can NOT be added to the repository: ``.png, .tif, .npy, .pkl, .dat, .html``
   If you have doubts about the files you are committing, you can always ask for help.
2. Branch naming follows the pattern ``<#>-feature_description`` where ``<#>`` represents the issue number.
   For example: ``1-new_feature``.
3. You need to have implemented tests for your code before merging into develop.
4. You need to run all the tests before opening a pull request.

## Code style
Coding style is enforced with pre-commit hooks, you should install them by doing the following in the root directory:

```bash
sudo apt install pre-commit
pre-commit install
```

The docstrings follow the ``Doxygen`` format, while the code style follows the ``Microsoft`` style.

## Tests
As mentioned before, your code should ideally be tested thoroughly. For this we use `GoogleTests`. You can find
examples of it in the already created unittests in our repository.
To run all tests you can run ``make test`` from the build directory after correctly configuring it.

## Documentation
Ideally all your code should be documented, the markup used for the documentation is [Doxygen-style](https://www
.doxygen.nl/manual/docblocks.html). You should stick to it.
To build the documentation locally and check that yours is rendered properly, you need to follow the next steps.

``` bash
conda create -n xilens python=3.10
conda activate xilens
pip install mkdocs mkdocs-glightbox mkdocs-material mkdocstrings[python]
mkdocs serve
```

A `url` will be displayed in the terminal, which you can open in your browser to visualize the documentation.
This documentation is also hosted online: [`XiLens` documentation](https://xilens.readthedocs.io/en/latest/).

## Contributing a feature/bug fix

If you have doubts on how to finish your feature branch, you can always ask for help

1. Create issue on [GitHub](https://github.com/IMSY-DKFZ/xilens/issues), if an issue does not exist yet.
2. Assign the issue to you.
3. Create a fork of the repository.
4. Create a new branch based from the``develop`` branch.

    !!! tip "Branch naming"

        The ``branch name`` has to match the following pattern: ``<#>-<short_description>``, For example: ``1-new_feature``.

5. Implement your feature
6. Update your branch: ``git checkout <branch_name> && git merge develop``.
7. Run tests in your branch
8. Create a pull request for your branch. select ``develop`` as the destination branch.
9. The branch will be reviewed and automatically merged if there are no requested changes.

## Finished or deprecated branches
Branches that are no longer in use or that have been merged into develop should be deleted. This process is automated in
GitHub, however, if you merged into develop but your branch is still there, you can delete it manually:

!!! warning "Delete branches safely"

    Make sure that your code is already in origin/develop (before deleting branches manually) .

```bash
git branch -d <branch-name>
git push origin --delete <branch-name>
```
