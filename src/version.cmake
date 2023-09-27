set(SUSI_REPO "${PROJECT_SOURCE_DIR}/.git")
message("Checking hash of repo: ${SUSI_REPO}")
execute_process(COMMAND bash -c "git --git-dir=${SUSI_REPO} log --pretty=format:'%h' -n 1"
        OUTPUT_VARIABLE GIT_REV
)
# Check whether we got any revision (which isn't
# always the case, e.g. when someone downloaded a zip
# file from Github instead of a checkout)
if ("${GIT_REV}" STREQUAL "")
    set(GIT_REV "N/A")
    set(GIT_DIFF "")
    set(GIT_TAG "N/A")
    set(GIT_BRANCH "N/A")
else ()
    execute_process(
            COMMAND bash -c "git --git-dir=${SUSI_REPO} diff --quiet --exit-code || echo +"
            OUTPUT_VARIABLE GIT_DIFF)
    execute_process(
            COMMAND bash -c "git --git-dir=${SUSI_REPO} describe --exact-match --tags"
            OUTPUT_VARIABLE GIT_TAG ERROR_QUIET)
    execute_process(
            COMMAND bash -c "git --git-dir=${SUSI_REPO} rev-parse --abbrev-ref HEAD"
            OUTPUT_VARIABLE GIT_BRANCH)

    string(STRIP "${GIT_REV}" GIT_REV)
    string(SUBSTRING "${GIT_REV}" 1 7 GIT_REV)
    string(STRIP "${GIT_DIFF}" GIT_DIFF)
    string(STRIP "${GIT_TAG}" GIT_TAG)
    string(STRIP "${GIT_BRANCH}" GIT_BRANCH)
endif ()

set(VERSION "const char* GIT_REV=\"${GIT_REV}${GIT_DIFF}\";
const char* GIT_TAG=\"${GIT_TAG}\";
const char* GIT_BRANCH=\"${GIT_BRANCH}\";")

if (EXISTS version.cpp)
    file(READ version.cpp VERSION_)
else ()
    set(VERSION_ "")
endif ()

if (NOT "${VERSION}" STREQUAL "${VERSION_}")
    file(WRITE version.cpp "${VERSION}")
endif ()
