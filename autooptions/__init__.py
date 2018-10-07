#
# Copyright (C) 2017 Karl Linden
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the
#    distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

import optparse
import sys
from waflib import Configure, Logs, Options, Utils

# A list of AutoOptions. It is local to each module, so all modules that
# use AutoOptions need to run both opt.load and conf.load. In contrast
# to the define and style options this does not need to and cannot be
# declared in the OptionsContext, because it is needed both for the
# options and the configure phase.
auto_options = []

class AutoOption:
    """
    This class represents an auto option that can be used in conjunction
    with the waf build system. By default it adds options --foo and
    --no-foo respectively to turn on or off foo respectively.
    Furthermore it incorporats logic and checks that are required for
    these features.

    An option can have an arbitrary number of dependencies that must be
    present for the option to be enabled. An option can be enabled or
    disabled by default. Here is how the logic works:
     1. If the option is explicitly disabled, through --no-foo, then no
        checks are made and the option is disabled.
     2. If the option is explicitly enabled, through --foo, then check
        for all required dependencies, and if some of them are not
        found, then print a fatal error telling the user there were
        dependencies missing.
     3. Otherwise, if the option is enabled by default, then check for
        all dependencies. If all dependencies are found the option is
        enabled. Otherwise it is disabled.
     4. Lastly, if no option was given and the option is disabled by
        default, then no checks are performed and the option is
        disabled.

    To add a dependency to an option use the check, check_cfg and
    find_program methods of this class. The methods are merely small
    wrappers around their configuration context counterparts and behave
    identically. Note that adding dependencies is done in the options
    phase and not in the configure phase, although the checks are
    acutally executed during the configure phase.

    Custom check functions can be added using the add_function method.
    As with the other checks the check function will be invoked during
    the configuration. Refer to the documentation of the add_function
    method for details.

    When all checks have been made and the class has made a decision the
    result is saved in conf.env['NAME'] where 'NAME' by default is the
    uppercase of the name argument to __init__, with hyphens replaced by
    underscores. This default can be changed with the conf_dest argument
    to __init__.

    The class will define a preprocessor symbol with the result. The
    default name is WITH_NAME, to not collide with the standard define
    of check_cfg, but it can be changed using the define argument to
    __init__. It can also be changed globally using
    set_auto_options_define.
    """

    def __init__(self, opt, name, help=None, default=True,
            conf_dest=None, define=None, style=None):
        """
        Class initializing method.

        Arguments:
            opt       OptionsContext
            name      name of the option, e.g. alsa
            help      help text that will be displayed in --help output
            conf_dest conf.env variable to define to the result
            define    the preprocessor symbol to define with the result
            style     the option style to use; see below for options
        """

        # The dependencies to check for. The elements are on the form
        # (func, k, kw) where func is the function or function name that
        # is used for the check and k and kw are the arguments and
        # options to give the function.
        self.deps = []

        # Whether or not the option should be enabled. None indicates
        # that the checks have not been performed yet.
        self.enable = None

        self.help = help
        if help:
            if default:
                help_comment = ' (enabled by default if possible)'
            else:
                help_comment = ' (disabled by default)'
            option_help = help + help_comment
            no_option_help = None
        else:
            option_help = no_option_help = optparse.SUPPRESS_HELP

        self.dest = 'auto_option_' + name

        self.default = default

        safe_name = Utils.quote_define_name(name)
        self.conf_dest = conf_dest or safe_name

        default_define = opt.get_auto_options_define()
        self.define = define or default_define % safe_name

        if not style:
            style = opt.get_auto_options_style()
        self.style = style

        # plain (default):
        #   --foo | --no-foo
        # yesno:
        #   --foo=yes | --foo=no
        # yesno_and_hack:
        #  --foo[=yes] | --foo=no or --no-foo
        # enable:
        #  --enable-foo | --disble-foo
        # with:
        #  --with-foo | --without-foo
        if style in ['plain', 'yesno', 'yesno_and_hack']:
            self.no_option = '--no-' + name
            self.yes_option = '--' + name
        elif style == 'enable':
            self.no_option = '--disable-' + name
            self.yes_option = '--enable-' + name
        elif style == 'with':
            self.no_option = '--without-' + name
            self.yes_option = '--with-' + name
        else:
            opt.fatal('invalid style')

        if style in ['yesno', 'yesno_and_hack']:
            opt.add_option(
                    self.yes_option,
                    dest=self.dest,
                    action='store',
                    choices=['auto', 'no', 'yes'],
                    default='auto',
                    help=option_help,
                    metavar='no|yes')
        else:
            opt.add_option(
                    self.yes_option,
                    dest=self.dest,
                    action='store_const',
                    const='yes',
                    default='auto',
                    help=option_help)
            opt.add_option(
                    self.no_option,
                    dest=self.dest,
                    action='store_const',
                    const='no',
                    default='auto',
                    help=no_option_help)

    def check(self, *k, **kw):
        self.deps.append(('check', k, kw))
    def check_cfg(self, *k, **kw):
        self.deps.append(('check_cfg', k, kw))
    def find_program(self, *k, **kw):
        self.deps.append(('find_program', k, kw))

    def add_function(self, func, *k, **kw):
        """
        Add a custom function to be invoked as part of the
        configuration. During the configuration the function will be
        invoked with the configuration context as first argument
        followed by the arugments to this method, except for the func
        argument. The function must print a 'Checking for...' message,
        because it is referred to if the check fails and this option is
        requested.

        On configuration error the function shall raise
        conf.errors.ConfigurationError.
        """
        self.deps.append((func, k, kw))

    def _check(self, conf, required):
        """
        This private method checks all dependencies. It checks all
        dependencies (even if some dependency was not found) so that the
        user can install all missing dependencies in one go, instead of
        playing the infamous hit-configure-hit-configure game.

        This function returns True if all dependencies were found and
        False otherwise.
        """
        all_found = True

        for (f,k,kw) in self.deps:
            if hasattr(f, '__call__'):
                # This is a function supplied by add_function.
                func = f
                k = list(k)
                k.insert(0, conf)
                k = tuple(k)
            else:
                func = getattr(conf, f)

            try:
                func(*k, **kw)
            except conf.errors.ConfigurationError:
                all_found = False
                if required:
                    Logs.error('The above check failed, but the '
                               'checkee is required for %s.' %
                               self.yes_option)

        return all_found

    def configure(self, conf):
        """
        This function configures the option examining the command line
        option. It sets self.enable to whether this options should be
        enabled or not, that is True or False respectively. If not all
        dependencies were found self.enable will be False.
        conf.env['NAME'] and a preprocessor symbol will be defined with
        the result.

        If the option was desired but one or more dependencies were not
        found the an error message will be printed for each missing
        dependency.

        This function returns True on success and False on if the option
        was requested but cannot be enabled.
        """
        # If the option has already been configured once, do not
        # configure it again.
        if self.enable != None:
            return True

        argument = getattr(Options.options, self.dest)
        if argument == 'no':
            self.enable = False
            retvalue = True
        elif argument == 'yes':
            retvalue = self.enable = self._check(conf, True)
        else:
            self.enable = self.default and self._check(conf, False)
            retvalue = True

        conf.env[self.conf_dest] = self.enable
        conf.define(self.define, int(self.enable))
        return retvalue

    def summarize(self, conf):
        """
        This function displays a result summary with the help text and
        the result of the configuration.
        """
        if self.help:
            if self.enable:
                conf.msg(self.help, 'yes', color='GREEN')
            else:
                conf.msg(self.help, 'no', color='YELLOW')

def options(opt):
    """
    This function declares necessary variables in the option context.
    The reason for saving variables in the option context is to allow
    autooptions to be loaded from modules (which will receive a new
    instance of this module, clearing any global variables) with a
    uniform style and default in the entire project.

    Call this function through opt.load('autooptions').
    """
    if not hasattr(opt, 'auto_options_style'):
        opt.auto_options_style = 'plain'
    if not hasattr(opt, 'auto_options_define'):
        opt.auto_options_define = 'WITH_%s'

def opt(f):
    """
    Decorator: attach a new option function to Options.OptionsContext.

    :param f: method to bind
    :type f: function
    """
    setattr(Options.OptionsContext, f.__name__, f)

@opt
def add_auto_option(self, *k, **kw):
    """
    This function adds an AutoOption to the options context. It takes
    the same arguments as the initializer funtion of the AutoOptions
    class.
    """
    option = AutoOption(self, *k, **kw)
    auto_options.append(option)
    return option

@opt
def get_auto_options_define(self):
    """
    This function gets the default define name. This default can be
    changed through set_auto_optoins_define.
    """
    return self.auto_options_define

@opt
def set_auto_options_define(self, define):
    """
    This function sets the default define name. The default is
    'WITH_%s', where %s will be replaced with the name of the option in
    uppercase.
    """
    self.auto_options_define = define

@opt
def get_auto_options_style(self):
    """
    This function gets the default option style, which will be used for
    the subsequent options.
    """
    return self.auto_options_style

@opt
def set_auto_options_style(self, style):
    """
    This function sets the default option style, which will be used for
    the subsequent options.
    """
    self.auto_options_style = style

@opt
def apply_auto_options_hack(self):
    """
    This function applies the hack necessary for the yesno_and_hack
    option style. The hack turns --foo into --foo=yes and --no-foo into
    --foo=no.

    It must be called before options are parsed, that is before the
    configure phase.
    """
    for option in auto_options:
        # With the hack the yesno options simply extend plain options.
        if option.style == 'yesno_and_hack':
            for i in range(1, len(sys.argv)):
                if sys.argv[i] == option.yes_option:
                    sys.argv[i] = option.yes_option + '=yes'
                elif sys.argv[i] == option.no_option:
                    sys.argv[i] = option.yes_option + '=no'

@Configure.conf
def summarize_auto_options(self):
    """
    This function prints a summary of the configuration of the auto
    options. Obviously, it must be called after
    conf.load('autooptions').
    """
    for option in auto_options:
        option.summarize(self)

def configure(conf):
    """
    This configures all auto options. Call it through
    conf.load('autooptions').
    """
    ok = True
    for option in auto_options:
        if not option.configure(conf):
            ok = False
    if not ok:
        conf.fatal('Some requested options had unsatisfied ' +
                'dependencies.\n' +
                'See the above configuration for details.')
