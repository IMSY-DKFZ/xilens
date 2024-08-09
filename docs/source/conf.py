# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = "xilens"
copyright = "2024, Leonardo Ayala (German Cancer Research Center, Intelligent Medical Systems (IMSY))"
author = "Leonardo Ayala (German Cancer Research Center, Intelligent Medical Systems (IMSY))"
# The short X.Y version.
version = u"0.2"
# The full version, including alpha/beta/rc tags.
release = u"0.2.1"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = ["sphinx.ext.todo", "breathe", "sphinx.ext.graphviz"]

templates_path = ["_templates"]
exclude_patterns = []

# -- TODOs configuration -----------------------------------------------------
todo_include_todos = False

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output
html_theme = "furo"
html_logo = "../../resources/icon.png"
html_favicon = "../../resources/icon.png"

# -- breathe configuration ---------------------------------------------------
breathe_projects = {"xilens": "../doxygen/xml/"}
breathe_default_project = "xilens"
breathe_default_members = ("members", "undoc-members", "protected-members", "private-members", "membergroups")
