#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Empirical documentation build configuration file, created by
# sphinx-quickstart on Fri Oct 16 21:33:50 2015.
#
# This file is execfile()d with the current directory set to its
# containing dir.
#
# Note that not all possible configuration values are present in this
# autogenerated file.
#
# All configuration values have a default; values that are commented out
# serve to show the default.

# If extensions (or modules to document with autodoc) are in another
# directory, add these directories to sys.path here. If the directory is
# relative to the documentation root, use os.path.abspath to make it
# absolute, like shown here.
#
import glob
import os
import sphinx_rtd_theme
import subprocess
import sys
import textwrap

# -- General configuration ---------------------------------------------

# If your documentation needs a minimal Sphinx version, state it here.
#
# needs_sphinx = '1.0'

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom ones.
extensions = [
    'sphinx.ext.viewcode',
    'sphinx.ext.mathjax',
    'sphinx.ext.todo',
    'sphinx_rtd_theme',
    'breathe',
    'myst_parser',
    'sphinx_tippy',
    'sphinxcontrib.bibtex'
]

myst_heading_anchors = 4

bibtex_bibfiles = ['bibliography.bib']
bibtex_reference_style = "author_year"
bibtex_default_style = 'unsrt'

# Setup the breathe extension
breathe_projects = {
    "Empirical": "./doxyoutput/xml"
}
breathe_default_project = "Empirical"

breathe_projects_source = {
    "Empirical" : (
        "../include/emp", glob.glob("**/*.hpp")
    )
}

breathe_doxygen_config_options = {'PREDEFINED': 'DOXYGEN_SHOULD_SKIP_THIS'}

# cpp_index_common_prefix = ["emp", "emp::", "emp::web::", "web", "emp::web"]

# Tell sphinx what the primary language being documented is.
primary_domain = 'cpp'

# Tell sphinx what the pygments highlight language should be.
highlight_language = 'cpp'

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# The suffix(es) of source filenames.
# You can specify multiple suffix as a list of string:
# source_suffix = ['.rst', '.md']
source_suffix = ['.rst', '.md']

# The master toctree document.
master_doc = 'index'

# General information about the project.
project = u'Empirical'
copyright = u"2015-2023, Charles Ofria"
author = u"Charles Ofria"

# The version info for the project you're documenting, acts as replacement
# for |version| and |release|, also used in various other places throughout
# the built documents.
#
# The short X.Y version.
version = '1.1.3'
# The full version, including alpha/beta/rc tags.
release = '1.1.3'
# The language for content autogenerated by Sphinx. Refer to documentation
# for a list of supported languages.
#
# This is also used if you do content translation via gettext catalogs.
# Usually you set "language" from the command line for these cases.
language = "en"

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This patterns also effect to html_static_path and html_extra_path
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', 'docsvenv', 'in_progress']

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = 'sphinx'

# If true, `todo` and `todoList` produce output, else they produce nothing.
todo_include_todos = True

nitpick_ignore = [
    ('c:identifier', 'int32_t'),
    ('c:identifier', 'uint32_t'),
    ('c:identifier', 'int64_t'),
    ('c:identifier', 'uint64_t'),
    ('c:identifier', 'size_t'),
    ('c:identifier', 'bool'),
]

# -- Options for HTML output -------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'sphinx_rtd_theme'

# Theme options are theme-specific and customize the look and feel of a
# theme further.  For a list of options available for each theme, see the
# documentation.
#
html_theme_options = {
#    'collapse_navigation': False
}

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = []

html_copy_source = False
# -- Options for HTMLHelp output ---------------------------------------

# Output file base name for HTML help builder.
htmlhelp_basename = 'Empiricaldoc'


# -- Options for LaTeX output ------------------------------------------

latex_elements = {
    # The paper size ('letterpaper' or 'a4paper').
    #
    # 'papersize': 'letterpaper',

    # The font size ('10pt', '11pt' or '12pt').
    #
    # 'pointsize': '10pt',

    # Additional stuff for the LaTeX preamble.
    #
    # 'preamble': '',

    # Latex figure (float) alignment
    #
    # 'figure_align': 'htbp',
}

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title, author, documentclass
# [howto, manual, or own class]).
latex_documents = [
  (master_doc, 'Empirical.tex', u'Empirical Documentation',
   u'Charles Ofria', 'manual'),
]



# -- Options for manual page output ------------------------------------

# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = [
    (master_doc, 'empirical', u'Empirical Documentation',
     [author], 1)
]


# -- Options for Texinfo output ----------------------------------------

# Grouping the document tree into Texinfo files. List of tuples
# (source start file, target name, title, author,
#  dir menu entry, description, category)
texinfo_documents = [
  (master_doc, 'Empirical', u'Empirical Documentation',
   author, 'Empirical', 'A scientific software library for research, education, & public engagement.',
   'Miscellaneous'),
]

# -- Theme Options -------------------------------------------

# from https://exhale.readthedocs.io/en/latest/usage.html#start-to-finish-for-read-the-docs
# on_rtd is whether we are on readthedocs.org, this line of code grabbed from docs.readthedocs.org
on_rtd = os.environ.get('READTHEDOCS', None) == 'True'

if on_rtd: # rtd doesn't run Makefile, so we have to copy assets ourself
    subprocess.call(
        'mkdir -p _build/html/docs/; cp -r assets _build/html/docs/',
        shell=True,
    )
else:  # only import and set the theme if we're building docs locally
    import sphinx_rtd_theme
    html_theme = 'sphinx_rtd_theme'
    html_theme_path = [sphinx_rtd_theme.get_html_theme_path()]
    extensions.append('sphinx.ext.coverage')
