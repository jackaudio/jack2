# Operating System Compatibility Modules for WAF

This directory contains waf modules that aid compatibility across
different operating systems. Here a module is a pluggable and reusable
piece of code for the waf build system along with necessary
replacements.

To create a new compatibility module simply create a new subdirectory
containing a `wscript` file and any necessary replacement files. The
`wscript` must define the `options`, `configure` and `build` functions.

To use the modules you need to call `recurse` in your `options`,
`configure` and `build` commands. For example
```python
def options(opt):
    # Do stuff...
    opt.recurse('compat')
    # Do other stuff...

def configure(conf):
    # Do stuff...
    conf.recurse('compat')
    # Do other stuff...

def build(bld):
    # Do stuff...
    bld.recurse('compat')
    # Do other stuff...
```
assuming this directory is called `compat`. After doing this you need to
take any necessary actions described in the modules you want to use.

The code in this directory is inteded to be generic and reusable. When
writing new modules, please keep this in mind. Whenever necessary it
should be possible to make this directory a git submodule and all the
subdirectories other submodules, to aid reuse.

If you would like to use these modules in another project, please file
an issue so that we can join forces and maintain the compatabilitiy
modules in a separate repository.
