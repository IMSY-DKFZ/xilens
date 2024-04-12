The susicam library is only supported in Linux systems, although all dependencies are also available in Windows systems,
compilation in such systems has not been properly tested. 

# Building susicam from source
This software can be built following the instructions provided in out [Getting Started guide.](https://imsy.pages.dkfz.de/issi/susicam/getting_started.html)

# FAQ
If you are having issues, make sure to check the [FAQ section](https://imsy.pages.dkfz.de/issi/susicam/faq.html) of our documentation. 



---

## **Developer guidelines** :computer: 
<details>
<summary>
These guidelines are meant to help you develop code for this package: documentation, unittesting, bug fixes, features, releases, etc. (click to expand)

</summary>

### General rules
1. Binary files can NOT be added to the repository, ideally you should develop your code in `.cpp, .h` files.
   Example of files that should NOT be added to the repository: `.png`, `.tif`, `.npy`, `.pkl`, `.dat`, `.html`
   If you have doubts about the files you are committing, you can always ask for help.
2. Branch naming follows `git flow` conventions, you can find a cheatsheet with the most common commands here 
   [GitFlow](https://danielkummer.github.io/git-flow-cheatsheet/). 
   In order to initialize git flow in your environment you need to run:
   
   `git flow init -d`
   
   Before starting your `feature`, `hotfix`, etc.
   you need to create a task in the [SUSICAM work board](https://git.dkfz.de/imsy/issi/susicam/-/issues).
   After creating the task you will see a number like this `18` in the title of the task. You should use this for 
   naming your branch. For example: 
   
   `git flow feature start 18-my_branch_name`
   
   This will create a branch with the name `feature/18-my_branch_name`, notice that `git flow` prepends `feature` to 
   the branch name automatically. More details on how to finish the branch can be found  in the section [contributing a feature](#contributing-a-featurebug-fix)
3. You need to have implemented tests for your code before merging into develop, we use GoogleTests for that. You also need to have written documentation for your implementations.

### Documentation :book: 

Ideally all your code should be documented, the markup used for the documentation is [**Doxygen-style**](https://www.doxygen.nl/manual/docblocks.html). 
You should stick to it. The documentation can be built by doing the following. 
```bash
cd doc
sudo apt install -y doxygen graphviz
pip install -U sphinx furo breathe
doxygen Doxyfile
make html SPHINXOPTS="-j4"
```
After that, you should see a build directory where the html code is written to.


### Unittesting :test_tube: 

As mentioned before, your code should ideally be tested thoroughly. For this we use `unittests`. You can find examples of
it in the already created unittests in our repository. 

### Contributing a feature/bug fix :bug:
If you have doubts on how to finish your feature branch, you can always ask for help
1. Create issue on the [SUSICAM work board](https://git.dkfz.de/imsy/issi/susicam/-/issues), if a task does not exist yet.
2. Assign the task to you.
3. Create the `feature` branch: `git flow feature start <branch_name>`. 
   >`<branch_name>` has to match the following pattern: `<task_number>-<short_description_of_task>`
4. Update `feature` branch: `git checkout <branch_name> && git merge develop`
5. Run tests in `feature` branch
6. Create a merge request for your feature in gitlab, select `develop` and the destination branch and assign reviewers
7. The branch will be reviewed and automatically merged if there are no requested changes

### Contributing a hotfix :fire: 
The version tag has to match the following pattern: `v<x>.<y>.<z>` with `x=major, y=minor, z=patch` version number

1. Create a task on the [SUSICAM work board](https://git.dkfz.de/imsy/issi/susicam/-/issues), if a task does not exist yet.
2. Set the task priority to high, if not done yet.
3. Assign the task to you.
4. Create the `hotfix` branch: `git flow hotfix start <branch_name>`.
   > `<branch_name>` has to match the following pattern: `<task_number>-<short_description_of_task>`
5. Commit your hot fixes.
6. Bump the version number in the last commit. Increment the patch number of the latest version tag by one. Following files have to be touched:
    * `doc/conf.py`
7. Finish the `hotfix` by specifying the version tag and the message: `git flow hotfix finish <task_number>-<branch_name> -T <version_tag> -m "Release of susi package"`
8. The `hotfix` branch has been merged into `main` and `develop`. The `hotfix` branch has been deleted locally and remotely. You are now on `develop` branch, which has to be pushed.
9. Switch to `main` branch and also push it.
10. Also push the version tag: `git push origin <version_tag>`, or alternatively `git push --tags`
11. Change status of the task to `resolved`.

## Preparing a release :package: 
The version tag has to match the following pattern: `v<x>.<y>.<z>` with `x=major, y=minor, z=patch` version number

1. Releases are tracked via [milestones](https://git.dkfz.de/imsy/issi/susicam/-/milestones).
2. Make sure that all tasks relevant for the corresponding milestone have been resolved.
3. Create a `release` branch: `git flow release start <version_tag>`. The version tag is the title of the milestone.
4. Perform testing on the `release` branch.
    * run tests
    * Fix issues on the `release` branch
    * Repeat testing procedure until no errors occur
5. Update release number. Following files have to be touched:
    * `doc/conf.py`
6. Finish a release: `git flow release finish <branch_name> -T <version_tag> -m "Release of susicam package"`
7. Push `develop` branch and `master` branch.
8. Push tags with `git push origin <version_tag>`, or alternatively `git push --tags`
9. Change status of all release-relevant tasks to resolved (if not done already).

### Finished or deprecated branches :older_adult: 

Branches that are no longer in use or that have been merged into develop should be deleted, except persistent branches such as `documentation`.
If you followed the guidelines for finishing a feature, your feature branch should have been deleted automatically.
If you merged into develop but your branch is still there, you can delete it manually by doing 

>Make sure that your code is already in develop (before delete branches) by going to [susicam](https://git.dkfz.de/imsy/issi/susicam) and checking the develop branch

`git branch -d <branch-name>`
`git push origin --delete <branch-name>`

</details>
