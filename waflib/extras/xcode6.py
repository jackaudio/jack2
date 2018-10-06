#! /usr/bin/env python
# encoding: utf-8
# XCode 3/XCode 4 generator for Waf
# Based on work by Nicolas Mercier 2011
# Extended by Simon Warg 2015, https://github.com/mimon
# XCode project file format based on http://www.monobjc.net/xcode-project-file-format.html

"""
Usage:

See also demos/xcode6/ folder

def options(opt):
	opt.load('xcode6')

def configure(cnf):
	# <do your stuff>

	# For example
	cnf.env.SDKROOT = 'macosx10.9'

	# Use cnf.PROJ_CONFIGURATION to completely set/override
	# global project settings
	# cnf.env.PROJ_CONFIGURATION = {
	# 	'Debug': {
	# 		'SDKROOT': 'macosx10.9'
	# 	}
	# 	'MyCustomReleaseConfig': {
	# 		'SDKROOT': 'macosx10.10'
	# 	}
	# }

	# In the end of configure() do
	cnf.load('xcode6')

def build(bld):

	# Make a Framework target
	bld.framework(
		source_files={
			'Include': bld.path.ant_glob('include/MyLib/*.h'),
			'Source': bld.path.ant_glob('src/MyLib/*.cpp')
		},
		includes='include',
		export_headers=bld.path.ant_glob('include/MyLib/*.h'),
		target='MyLib',
	)

	# You can also make bld.dylib, bld.app, bld.stlib ...

$ waf configure xcode6
"""

# TODO: support iOS projects

from waflib import Context, TaskGen, Build, Utils, ConfigSet, Configure, Errors
from waflib.Build import BuildContext
import os, sys, random, time

HEADERS_GLOB = '**/(*.h|*.hpp|*.H|*.inl)'

MAP_EXT = {
	'': "folder",
	'.h' :  "sourcecode.c.h",

	'.hh':  "sourcecode.cpp.h",
	'.inl': "sourcecode.cpp.h",
	'.hpp': "sourcecode.cpp.h",

	'.c':   "sourcecode.c.c",

	'.m':   "sourcecode.c.objc",

	'.mm':  "sourcecode.cpp.objcpp",

	'.cc':  "sourcecode.cpp.cpp",

	'.cpp': "sourcecode.cpp.cpp",
	'.C':   "sourcecode.cpp.cpp",
	'.cxx': "sourcecode.cpp.cpp",
	'.c++': "sourcecode.cpp.cpp",

	'.l':   "sourcecode.lex", # luthor
	'.ll':  "sourcecode.lex",

	'.y':   "sourcecode.yacc",
	'.yy':  "sourcecode.yacc",

	'.plist': "text.plist.xml",
	".nib":   "wrapper.nib",
	".xib":   "text.xib",
}

# Used in PBXNativeTarget elements
PRODUCT_TYPE_APPLICATION = 'com.apple.product-type.application'
PRODUCT_TYPE_FRAMEWORK = 'com.apple.product-type.framework'
PRODUCT_TYPE_EXECUTABLE = 'com.apple.product-type.tool'
PRODUCT_TYPE_LIB_STATIC = 'com.apple.product-type.library.static'
PRODUCT_TYPE_LIB_DYNAMIC = 'com.apple.product-type.library.dynamic'
PRODUCT_TYPE_EXTENSION = 'com.apple.product-type.kernel-extension'
PRODUCT_TYPE_IOKIT = 'com.apple.product-type.kernel-extension.iokit'

# Used in PBXFileReference elements
FILE_TYPE_APPLICATION = 'wrapper.cfbundle'
FILE_TYPE_FRAMEWORK = 'wrapper.framework'
FILE_TYPE_LIB_DYNAMIC = 'compiled.mach-o.dylib'
FILE_TYPE_LIB_STATIC = 'archive.ar'
FILE_TYPE_EXECUTABLE = 'compiled.mach-o.executable'

# Tuple packs of the above
TARGET_TYPE_FRAMEWORK = (PRODUCT_TYPE_FRAMEWORK, FILE_TYPE_FRAMEWORK, '.framework')
TARGET_TYPE_APPLICATION = (PRODUCT_TYPE_APPLICATION, FILE_TYPE_APPLICATION, '.app')
TARGET_TYPE_DYNAMIC_LIB = (PRODUCT_TYPE_LIB_DYNAMIC, FILE_TYPE_LIB_DYNAMIC, '.dylib')
TARGET_TYPE_STATIC_LIB = (PRODUCT_TYPE_LIB_STATIC, FILE_TYPE_LIB_STATIC, '.a')
TARGET_TYPE_EXECUTABLE = (PRODUCT_TYPE_EXECUTABLE, FILE_TYPE_EXECUTABLE, '')

# Maps target type string to its data
TARGET_TYPES = {
	'framework': TARGET_TYPE_FRAMEWORK,
	'app': TARGET_TYPE_APPLICATION,
	'dylib': TARGET_TYPE_DYNAMIC_LIB,
	'stlib': TARGET_TYPE_STATIC_LIB,
	'exe' :TARGET_TYPE_EXECUTABLE,
}

"""
Configuration of the global project settings. Sets an environment variable 'PROJ_CONFIGURATION'
which is a dictionary of configuration name and buildsettings pair.
E.g.:
env.PROJ_CONFIGURATION = {
	'Debug': {
		'ARCHS': 'x86',
		...
	}
	'Release': {
		'ARCHS' x86_64'
		...
	}
}
The user can define a completely customized dictionary in configure() stage. Otherwise a default Debug/Release will be created
based on env variable
"""
def configure(self):
	if not self.env.PROJ_CONFIGURATION:
		self.to_log("A default project configuration was created since no custom one was given in the configure(conf) stage. Define your custom project settings by adding PROJ_CONFIGURATION to env. The env.PROJ_CONFIGURATION must be a dictionary with at least one key, where each key is the configuration name, and the value is a dictionary of key/value settings.\n")

	# Check for any added config files added by the tool 'c_config'.
	if 'cfg_files' in self.env:
		self.env.INCLUDES = Utils.to_list(self.env.INCLUDES) + [os.path.abspath(os.path.dirname(f)) for f in self.env.cfg_files]

	# Create default project configuration?
	if 'PROJ_CONFIGURATION' not in self.env:
		self.env.PROJ_CONFIGURATION = {
			"Debug": self.env.get_merged_dict(),
			"Release": self.env.get_merged_dict(),
		}

	# Some build settings are required to be present by XCode. We will supply default values
	# if user hasn't defined any.
	defaults_required = [('PRODUCT_NAME', '$(TARGET_NAME)')]
	for cfgname,settings in self.env.PROJ_CONFIGURATION.iteritems():
		for default_var, default_val in defaults_required:
			if default_var not in settings:
				settings[default_var] = default_val

	# Error check customization
	if not isinstance(self.env.PROJ_CONFIGURATION, dict):
		raise Errors.ConfigurationError("The env.PROJ_CONFIGURATION must be a dictionary with at least one key, where each key is the configuration name, and the value is a dictionary of key/value settings.")

part1 = 0
part2 = 10000
part3 = 0
id = 562000999
def newid():
	global id
	id = id + 1
	return "%04X%04X%04X%012d" % (0, 10000, 0, id)

class XCodeNode:
	def __init__(self):
		self._id = newid()
		self._been_written = False

	def tostring(self, value):
		if isinstance(value, dict):
			result = "{\n"
			for k,v in value.items():
				result = result + "\t\t\t%s = %s;\n" % (k, self.tostring(v))
			result = result + "\t\t}"
			return result
		elif isinstance(value, str):
			return "\"%s\"" % value
		elif isinstance(value, list):
			result = "(\n"
			for i in value:
				result = result + "\t\t\t%s,\n" % self.tostring(i)
			result = result + "\t\t)"
			return result
		elif isinstance(value, XCodeNode):
			return value._id
		else:
			return str(value)

	def write_recursive(self, value, file):
		if isinstance(value, dict):
			for k,v in value.items():
				self.write_recursive(v, file)
		elif isinstance(value, list):
			for i in value:
				self.write_recursive(i, file)
		elif isinstance(value, XCodeNode):
			value.write(file)

	def write(self, file):
		if not self._been_written:
			self._been_written = True
			for attribute,value in self.__dict__.items():
				if attribute[0] != '_':
					self.write_recursive(value, file)
			w = file.write
			w("\t%s = {\n" % self._id)
			w("\t\tisa = %s;\n" % self.__class__.__name__)
			for attribute,value in self.__dict__.items():
				if attribute[0] != '_':
					w("\t\t%s = %s;\n" % (attribute, self.tostring(value)))
			w("\t};\n\n")

# Configurations
class XCBuildConfiguration(XCodeNode):
	def __init__(self, name, settings = {}, env=None):
		XCodeNode.__init__(self)
		self.baseConfigurationReference = ""
		self.buildSettings = settings
		self.name = name
		if env and env.ARCH:
			settings['ARCHS'] = " ".join(env.ARCH)


class XCConfigurationList(XCodeNode):
	def __init__(self, configlst):
		""" :param configlst: list of XCConfigurationList """
		XCodeNode.__init__(self)
		self.buildConfigurations = configlst
		self.defaultConfigurationIsVisible = 0
		self.defaultConfigurationName = configlst and configlst[0].name or ""

# Group/Files
class PBXFileReference(XCodeNode):
	def __init__(self, name, path, filetype = '', sourcetree = "SOURCE_ROOT"):
		XCodeNode.__init__(self)
		self.fileEncoding = 4
		if not filetype:
			_, ext = os.path.splitext(name)
			filetype = MAP_EXT.get(ext, 'text')
		self.lastKnownFileType = filetype
		self.name = name
		self.path = path
		self.sourceTree = sourcetree

	def __hash__(self):
		return (self.path+self.name).__hash__()

	def __eq__(self, other):
		return (self.path, self.name) == (other.path, other.name)

class PBXBuildFile(XCodeNode):
	""" This element indicate a file reference that is used in a PBXBuildPhase (either as an include or resource). """
	def __init__(self, fileRef, settings={}):
		XCodeNode.__init__(self)
		
		# fileRef is a reference to a PBXFileReference object
		self.fileRef = fileRef

		# A map of key/value pairs for additionnal settings.
		self.settings = settings

	def __hash__(self):
		return (self.fileRef).__hash__()

	def __eq__(self, other):
		return self.fileRef == other.fileRef

class PBXGroup(XCodeNode):
	def __init__(self, name, sourcetree = "<group>"):
		XCodeNode.__init__(self)
		self.children = []
		self.name = name
		self.sourceTree = sourcetree

	def add(self, sources):
		""" sources param should be a list of PBXFileReference objects """
		self.children.extend(sources)

class PBXContainerItemProxy(XCodeNode):
	""" This is the element for to decorate a target item. """
	def __init__(self, containerPortal, remoteGlobalIDString, remoteInfo='', proxyType=1):
		XCodeNode.__init__(self)
		self.containerPortal = containerPortal # PBXProject
		self.remoteGlobalIDString = remoteGlobalIDString # PBXNativeTarget
		self.remoteInfo = remoteInfo # Target name
		self.proxyType = proxyType
		

class PBXTargetDependency(XCodeNode):
	""" This is the element for referencing other target through content proxies. """
	def __init__(self, native_target, proxy):
		XCodeNode.__init__(self)
		self.target = native_target
		self.targetProxy = proxy
		
class PBXFrameworksBuildPhase(XCodeNode):
	""" This is the element for the framework link build phase, i.e. linking to frameworks """
	def __init__(self, pbxbuildfiles):
		XCodeNode.__init__(self)
		self.buildActionMask = 2147483647
		self.runOnlyForDeploymentPostprocessing = 0
		self.files = pbxbuildfiles #List of PBXBuildFile (.o, .framework, .dylib)

class PBXHeadersBuildPhase(XCodeNode):
	""" This is the element for adding header files to be packaged into the .framework """
	def __init__(self, pbxbuildfiles):
		XCodeNode.__init__(self)
		self.buildActionMask = 2147483647
		self.runOnlyForDeploymentPostprocessing = 0
		self.files = pbxbuildfiles #List of PBXBuildFile (.o, .framework, .dylib)

class PBXCopyFilesBuildPhase(XCodeNode):
	"""
	Represents the PBXCopyFilesBuildPhase section. PBXBuildFile
	can be added to this node to copy files after build is done.
	"""
	def __init__(self, pbxbuildfiles, dstpath, dstSubpathSpec=0, *args, **kwargs):
			XCodeNode.__init__(self)
			self.files = pbxbuildfiles
			self.dstPath = dstpath
			self.dstSubfolderSpec = dstSubpathSpec

class PBXSourcesBuildPhase(XCodeNode):
	""" Represents the 'Compile Sources' build phase in a Xcode target """
	def __init__(self, buildfiles):
		XCodeNode.__init__(self)
		self.files = buildfiles # List of PBXBuildFile objects

class PBXLegacyTarget(XCodeNode):
	def __init__(self, action, target=''):
		XCodeNode.__init__(self)
		self.buildConfigurationList = XCConfigurationList([XCBuildConfiguration('waf', {})])
		if not target:
			self.buildArgumentsString = "%s %s" % (sys.argv[0], action)
		else:
			self.buildArgumentsString = "%s %s --targets=%s" % (sys.argv[0], action, target)
		self.buildPhases = []
		self.buildToolPath = sys.executable
		self.buildWorkingDirectory = ""
		self.dependencies = []
		self.name = target or action
		self.productName = target or action
		self.passBuildSettingsInEnvironment = 0

class PBXShellScriptBuildPhase(XCodeNode):
	def __init__(self, action, target):
		XCodeNode.__init__(self)
		self.buildActionMask = 2147483647
		self.files = []
		self.inputPaths = []
		self.outputPaths = []
		self.runOnlyForDeploymentPostProcessing = 0
		self.shellPath = "/bin/sh"
		self.shellScript = "%s %s %s --targets=%s" % (sys.executable, sys.argv[0], action, target)

class PBXNativeTarget(XCodeNode):
	""" Represents a target in XCode, e.g. App, DyLib, Framework etc. """
	def __init__(self, target, node, target_type=TARGET_TYPE_APPLICATION, configlist=[], buildphases=[]):
		XCodeNode.__init__(self)
		product_type = target_type[0]
		file_type = target_type[1]

		self.buildConfigurationList = XCConfigurationList(configlist)
		self.buildPhases = buildphases
		self.buildRules = []
		self.dependencies = []
		self.name = target
		self.productName = target
		self.productType = product_type # See TARGET_TYPE_ tuples constants
		self.productReference = PBXFileReference(node.name, node.abspath(), file_type, '')

	def add_configuration(self, cf):
		""" :type cf: XCBuildConfiguration """
		self.buildConfigurationList.buildConfigurations.append(cf)

	def add_build_phase(self, phase):
		# Some build phase types may appear only once. If a phase type already exists, then merge them.
		if ( (phase.__class__ == PBXFrameworksBuildPhase)
			or (phase.__class__ == PBXSourcesBuildPhase) ):
			for b in self.buildPhases:
				if b.__class__ == phase.__class__:
					b.files.extend(phase.files)
					return
		self.buildPhases.append(phase)

	def add_dependency(self, depnd):
		self.dependencies.append(depnd)

# Root project object
class PBXProject(XCodeNode):
	def __init__(self, name, version, env):
		XCodeNode.__init__(self)

		if not isinstance(env.PROJ_CONFIGURATION, dict):
			raise Errors.WafError("Error: env.PROJ_CONFIGURATION must be a dictionary. This is done for you if you do not define one yourself. However, did you load the xcode module at the end of your wscript configure() ?")

		# Retreive project configuration
		configurations = []
		for config_name, settings in env.PROJ_CONFIGURATION.items():
			cf = XCBuildConfiguration(config_name, settings)
			configurations.append(cf)

		self.buildConfigurationList = XCConfigurationList(configurations)
		self.compatibilityVersion = version[0]
		self.hasScannedForEncodings = 1;
		self.mainGroup = PBXGroup(name)
		self.projectRoot = ""
		self.projectDirPath = ""
		self.targets = []
		self._objectVersion = version[1]

	def create_target_dependency(self, target, name):
		""" : param target : PXBNativeTarget """
		proxy = PBXContainerItemProxy(self, target, name)
		dependecy = PBXTargetDependency(target, proxy)
		return dependecy

	def write(self, file):

		# Make sure this is written only once
		if self._been_written:
			return

		w = file.write
		w("// !$*UTF8*$!\n")
		w("{\n")
		w("\tarchiveVersion = 1;\n")
		w("\tclasses = {\n")
		w("\t};\n")
		w("\tobjectVersion = %d;\n" % self._objectVersion)
		w("\tobjects = {\n\n")

		XCodeNode.write(self, file)

		w("\t};\n")
		w("\trootObject = %s;\n" % self._id)
		w("}\n")

	def add_target(self, target):
		self.targets.append(target)

	def get_target(self, name):
		""" Get a reference to PBXNativeTarget if it exists """
		for t in self.targets:
			if t.name == name:
				return t
		return None

class xcode(Build.BuildContext):
	cmd = 'xcode6'
	fun = 'build'

	file_refs = dict()
	build_files = dict()

	def as_nodes(self, files):
		""" Returns a list of waflib.Nodes from a list of string of file paths """
		nodes = []
		for x in files:
			if not isinstance(x, str):
				d = x
			else:
				d = self.srcnode.find_node(x)
			nodes.append(d)
		return nodes

	def create_group(self, name, files):
		"""
			Returns a new PBXGroup containing the files (paths) passed in the files arg 
			:type files: string
		"""
		group = PBXGroup(name)
		"""
		Do not use unique file reference here, since XCode seem to allow only one file reference
		to be referenced by a group.
		"""
		files = [(PBXFileReference(d.name, d.abspath())) for d in self.as_nodes(files)]
		group.add(files)
		return group

	def unique_filereference(self, fileref):
		"""
		Returns a unique fileref, possibly an existing one if the paths are the same.
		Use this after you've constructed a PBXFileReference to make sure there is
		only one PBXFileReference for the same file in the same project.
		"""
		if fileref not in self.file_refs:
			self.file_refs[fileref] = fileref
		return self.file_refs[fileref]

	def unique_buildfile(self, buildfile):
		"""
		Returns a unique buildfile, possibly an existing one.
		Use this after you've constructed a PBXBuildFile to make sure there is
		only one PBXBuildFile for the same file in the same project.
		"""
		if buildfile not in self.build_files:
			self.build_files[buildfile] = buildfile
		return self.build_files[buildfile]

	def execute(self):
		"""
		Entry point
		"""
		self.restore()
		if not self.all_envs:
			self.load_envs()
		self.recurse([self.run_dir])

		appname = getattr(Context.g_module, Context.APPNAME, os.path.basename(self.srcnode.abspath()))

		p = PBXProject(appname, ('Xcode 3.2', 46), self.env)
		
		# If we don't create a Products group, then
		# XCode will create one, which entails that
		# we'll start to see duplicate files in the UI
		# for some reason.
		products_group = PBXGroup('Products')
		p.mainGroup.children.append(products_group)

		for g in self.groups:
			for tg in g:
				if not isinstance(tg, TaskGen.task_gen):
					continue

				tg.post()

				target_group = PBXGroup(tg.name)
				p.mainGroup.children.append(target_group)

				# Determine what type to build - framework, app bundle etc.
				target_type = getattr(tg, 'target_type', 'app')
				if target_type not in TARGET_TYPES:
					raise Errors.WafError("Target type '%s' does not exists. Available options are '%s'. In target '%s'" % (target_type, "', '".join(TARGET_TYPES.keys()), tg.name))
				else:
					target_type = TARGET_TYPES[target_type]
				file_ext = target_type[2]

				# Create the output node
				target_node = tg.path.find_or_declare(tg.name+file_ext)
				
				target = PBXNativeTarget(tg.name, target_node, target_type, [], [])

				products_group.children.append(target.productReference)

				if hasattr(tg, 'source_files'):
					# Create list of PBXFileReferences
					sources = []
					if isinstance(tg.source_files, dict):
						for grpname,files in tg.source_files.items():
							group = self.create_group(grpname, files)
							target_group.children.append(group)
							sources.extend(group.children)
					elif isinstance(tg.source_files, list):
						group = self.create_group("Source", tg.source_files)
						target_group.children.append(group)
						sources.extend(group.children)
					else:
						self.to_log("Argument 'source_files' passed to target '%s' was not a dictionary. Hence, some source files may not be included. Please provide a dictionary of source files, with group name as key and list of source files as value.\n" % tg.name)

					supported_extensions = ['.c', '.cpp', '.m', '.mm']
					sources = filter(lambda fileref: os.path.splitext(fileref.path)[1] in supported_extensions, sources)
					buildfiles = [self.unique_buildfile(PBXBuildFile(fileref)) for fileref in sources]
					target.add_build_phase(PBXSourcesBuildPhase(buildfiles))

				# Create build settings which can override the project settings. Defaults to none if user
				# did not pass argument. However, this will be filled up further below with target specfic
				# search paths, libs to link etc.
				settings = getattr(tg, 'settings', {})

				# Check if any framework to link against is some other target we've made
				libs = getattr(tg, 'tmp_use_seen', [])
				for lib in libs:
					use_target = p.get_target(lib)
					if use_target:
						# Create an XCode dependency so that XCode knows to build the other target before this target
						target.add_dependency(p.create_target_dependency(use_target, use_target.name))
						target.add_build_phase(PBXFrameworksBuildPhase([PBXBuildFile(use_target.productReference)]))
						if lib in tg.env.LIB:
							tg.env.LIB = list(filter(lambda x: x != lib, tg.env.LIB))

				# If 'export_headers' is present, add files to the Headers build phase in xcode.
				# These are files that'll get packed into the Framework for instance.
				exp_hdrs = getattr(tg, 'export_headers', [])
				hdrs = self.as_nodes(Utils.to_list(exp_hdrs))
				files = [self.unique_filereference(PBXFileReference(n.name, n.abspath())) for n in hdrs]
				target.add_build_phase(PBXHeadersBuildPhase([PBXBuildFile(f, {'ATTRIBUTES': ('Public',)}) for f in files]))

				# Install path
				installpaths = Utils.to_list(getattr(tg, 'install', []))
				prodbuildfile = PBXBuildFile(target.productReference)
				for instpath in installpaths:
					target.add_build_phase(PBXCopyFilesBuildPhase([prodbuildfile], instpath))

				# Merge frameworks and libs into one list, and prefix the frameworks
				ld_flags = ['-framework %s' % lib.split('.framework')[0] for lib in Utils.to_list(tg.env.FRAMEWORK)]
				ld_flags.extend(Utils.to_list(tg.env.STLIB) + Utils.to_list(tg.env.LIB))

				# Override target specfic build settings
				bldsettings = {
					'HEADER_SEARCH_PATHS': ['$(inherited)'] + tg.env['INCPATHS'],
					'LIBRARY_SEARCH_PATHS': ['$(inherited)'] + Utils.to_list(tg.env.LIBPATH) + Utils.to_list(tg.env.STLIBPATH),
					'FRAMEWORK_SEARCH_PATHS': ['$(inherited)'] + Utils.to_list(tg.env.FRAMEWORKPATH),
					'OTHER_LDFLAGS': r'\n'.join(ld_flags)
				}

				# The keys represents different build configuration, e.g. Debug, Release and so on..
				# Insert our generated build settings to all configuration names
				keys = set(settings.keys() + self.env.PROJ_CONFIGURATION.keys())
				for k in keys:
					if k in settings:
						settings[k].update(bldsettings)
					else:
						settings[k] = bldsettings

				for k,v in settings.items():
					target.add_configuration(XCBuildConfiguration(k, v))

				p.add_target(target)
				
		node = self.bldnode.make_node('%s.xcodeproj' % appname)
		node.mkdir()
		node = node.make_node('project.pbxproj')
		p.write(open(node.abspath(), 'w'))
	
	def build_target(self, tgtype, *k, **kw):
		"""
		Provide user-friendly methods to build different target types
		E.g. bld.framework(source='..', ...) to build a Framework target.
		E.g. bld.dylib(source='..', ...) to build a Dynamic library target. etc...
		"""
		self.load('ccroot')
		kw['features'] = 'cxx cxxprogram'
		kw['target_type'] = tgtype
		return self(*k, **kw)

	def app(self, *k, **kw): return self.build_target('app', *k, **kw)
	def framework(self, *k, **kw): return self.build_target('framework', *k, **kw)
	def dylib(self, *k, **kw): return self.build_target('dylib', *k, **kw)
	def stlib(self, *k, **kw): return self.build_target('stlib', *k, **kw)
	def exe(self, *k, **kw): return self.build_target('exe', *k, **kw)
