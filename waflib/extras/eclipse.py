#! /usr/bin/env python
# encoding: utf-8
# Eclipse CDT 5.0 generator for Waf
# Richard Quirk 2009-1011 (New BSD License)
# Thomas Nagy 2011 (ported to Waf 1.6)

"""
Usage:

def options(opt):
	opt.load('eclipse')

$ waf configure eclipse
"""

import sys, os
from waflib import Utils, Logs, Context, Options, Build, TaskGen, Scripting
from xml.dom.minidom import Document

STANDARD_INCLUDES = [ '/usr/local/include', '/usr/include' ]

oe_cdt = 'org.eclipse.cdt'
cdt_mk = oe_cdt + '.make.core'
cdt_core = oe_cdt + '.core'
cdt_bld = oe_cdt + '.build.core'

class eclipse(Build.BuildContext):
	cmd = 'eclipse'
	fun = Scripting.default_cmd

	def execute(self):
		"""
		Entry point
		"""
		self.restore()
		if not self.all_envs:
			self.load_envs()
		self.recurse([self.run_dir])

		appname = getattr(Context.g_module, Context.APPNAME, os.path.basename(self.srcnode.abspath()))
		self.create_cproject(appname, pythonpath=self.env['ECLIPSE_PYTHON_PATH'])

	def create_cproject(self, appname, workspace_includes=[], pythonpath=[]):
		"""
		Create the Eclipse CDT .project and .cproject files
		@param appname The name that will appear in the Project Explorer
		@param build The BuildContext object to extract includes from
		@param workspace_includes Optional project includes to prevent
			  "Unresolved Inclusion" errors in the Eclipse editor
		@param pythonpath Optional project specific python paths
		"""
		source_dirs = []
		cpppath = self.env['CPPPATH']
		if sys.platform != 'win32':
			cpppath += STANDARD_INCLUDES
		Logs.warn('Generating Eclipse CDT project files')

		for g in self.groups:
			for tg in g:
				if not isinstance(tg, TaskGen.task_gen):
					continue

				tg.post()
				if not getattr(tg, 'link_task', None):
					continue

				l = Utils.to_list(getattr(tg, "includes", ''))
				sources = Utils.to_list(getattr(tg, 'source', ''))
				features = Utils.to_list(getattr(tg, 'features', ''))

				is_cc = 'c' in features or 'cxx' in features

				bldpath = tg.path.bldpath()

				base = os.path.normpath(os.path.join(self.bldnode.name, tg.path.srcpath()))

				if is_cc:
					sources_dirs = set([src.parent for src in tg.to_nodes(sources)])

				incnodes = tg.to_incnodes(tg.to_list(getattr(tg, 'includes', [])) + tg.env['INCLUDES'])
				for p in incnodes:
					path = p.path_from(self.srcnode)
					workspace_includes.append(path)

					if is_cc and path not in source_dirs:
						source_dirs.append(path)

		project = self.impl_create_project(sys.executable, appname)
		self.srcnode.make_node('.project').write(project.toprettyxml())

		waf = os.path.abspath(sys.argv[0])
		project = self.impl_create_cproject(sys.executable, waf, appname, workspace_includes, cpppath, source_dirs)
		self.srcnode.make_node('.cproject').write(project.toprettyxml())

		project = self.impl_create_pydevproject(appname, sys.path, pythonpath)
		self.srcnode.make_node('.pydevproject').write(project.toprettyxml())

	def impl_create_project(self, executable, appname):
		doc = Document()
		projectDescription = doc.createElement('projectDescription')
		self.add(doc, projectDescription, 'name', appname)
		self.add(doc, projectDescription, 'comment')
		self.add(doc, projectDescription, 'projects')
		buildSpec = self.add(doc, projectDescription, 'buildSpec')
		buildCommand = self.add(doc, buildSpec, 'buildCommand')
		self.add(doc, buildCommand, 'name', oe_cdt + '.managedbuilder.core.genmakebuilder')
		self.add(doc, buildCommand, 'triggers', 'clean,full,incremental,')
		arguments = self.add(doc, buildCommand, 'arguments')
		# the default make-style targets are overwritten by the .cproject values
		dictionaries = {
				cdt_mk + '.contents': cdt_mk + '.activeConfigSettings',
				cdt_mk + '.enableAutoBuild': 'false',
				cdt_mk + '.enableCleanBuild': 'true',
				cdt_mk + '.enableFullBuild': 'true',
				}
		for k, v in dictionaries.items():
			self.addDictionary(doc, arguments, k, v)

		natures = self.add(doc, projectDescription, 'natures')
		nature_list = """
			core.ccnature
			managedbuilder.core.ScannerConfigNature
			managedbuilder.core.managedBuildNature
			core.cnature
		""".split()
		for n in nature_list:
			self.add(doc, natures, 'nature', oe_cdt + '.' + n)

		self.add(doc, natures, 'nature', 'org.python.pydev.pythonNature')

		doc.appendChild(projectDescription)
		return doc

	def impl_create_cproject(self, executable, waf, appname, workspace_includes, cpppath, source_dirs=[]):
		doc = Document()
		doc.appendChild(doc.createProcessingInstruction('fileVersion', '4.0.0'))
		cconf_id = cdt_core + '.default.config.1'
		cproject = doc.createElement('cproject')
		storageModule = self.add(doc, cproject, 'storageModule',
				{'moduleId': cdt_core + '.settings'})
		cconf = self.add(doc, storageModule, 'cconfiguration', {'id':cconf_id})

		storageModule = self.add(doc, cconf, 'storageModule',
				{'buildSystemId': oe_cdt + '.managedbuilder.core.configurationDataProvider',
				 'id': cconf_id,
				 'moduleId': cdt_core + '.settings',
				 'name': 'Default'})

		self.add(doc, storageModule, 'externalSettings')

		extensions = self.add(doc, storageModule, 'extensions')
		extension_list = """
			VCErrorParser
			MakeErrorParser
			GCCErrorParser
			GASErrorParser
			GLDErrorParser
		""".split()
		ext = self.add(doc, extensions, 'extension',
					{'id': cdt_core + '.ELF', 'point':cdt_core + '.BinaryParser'})
		for e in extension_list:
			ext = self.add(doc, extensions, 'extension',
					{'id': cdt_core + '.' + e, 'point':cdt_core + '.ErrorParser'})

		storageModule = self.add(doc, cconf, 'storageModule',
				{'moduleId': 'cdtBuildSystem', 'version': '4.0.0'})
		config = self.add(doc, storageModule, 'configuration',
					{'artifactName': appname,
					 'id': cconf_id,
					 'name': 'Default',
					 'parent': cdt_bld + '.prefbase.cfg'})
		folderInfo = self.add(doc, config, 'folderInfo',
							{'id': cconf_id+'.', 'name': '/', 'resourcePath': ''})

		toolChain = self.add(doc, folderInfo, 'toolChain',
				{'id': cdt_bld + '.prefbase.toolchain.1',
				 'name': 'No ToolChain',
				 'resourceTypeBasedDiscovery': 'false',
				 'superClass': cdt_bld + '.prefbase.toolchain'})

		targetPlatform = self.add(doc, toolChain, 'targetPlatform',
				{ 'binaryParser': 'org.eclipse.cdt.core.ELF',
				  'id': cdt_bld + '.prefbase.toolchain.1', 'name': ''})

		waf_build = '"%s" %s'%(waf, eclipse.fun)
		waf_clean = '"%s" clean'%(waf)
		builder = self.add(doc, toolChain, 'builder',
						{'autoBuildTarget': waf_build,
						 'command': executable,
						 'enableAutoBuild': 'false',
						 'cleanBuildTarget': waf_clean,
						 'enableIncrementalBuild': 'true',
						 'id': cdt_bld + '.settings.default.builder.1',
						 'incrementalBuildTarget': waf_build,
						 'managedBuildOn': 'false',
						 'name': 'Gnu Make Builder',
						 'superClass': cdt_bld + '.settings.default.builder'})

		for tool_name in ("Assembly", "GNU C++", "GNU C"):
			tool = self.add(doc, toolChain, 'tool',
					{'id': cdt_bld + '.settings.holder.1',
					 'name': tool_name,
					 'superClass': cdt_bld + '.settings.holder'})
			if cpppath or workspace_includes:
				incpaths = cdt_bld + '.settings.holder.incpaths'
				option = self.add(doc, tool, 'option',
						{'id': incpaths+'.1',
						 'name': 'Include Paths',
						 'superClass': incpaths,
						 'valueType': 'includePath'})
				for i in workspace_includes:
					self.add(doc, option, 'listOptionValue',
								{'builtIn': 'false',
								'value': '"${workspace_loc:/%s/%s}"'%(appname, i)})
				for i in cpppath:
					self.add(doc, option, 'listOptionValue',
								{'builtIn': 'false',
								'value': '"%s"'%(i)})
			if tool_name == "GNU C++" or tool_name == "GNU C":
				self.add(doc,tool,'inputType',{ 'id':'org.eclipse.cdt.build.core.settings.holder.inType.1', \
					'languageId':'org.eclipse.cdt.core.gcc','languageName':tool_name, \
					'sourceContentType':'org.eclipse.cdt.core.cSource,org.eclipse.cdt.core.cHeader', \
					'superClass':'org.eclipse.cdt.build.core.settings.holder.inType' })
		if source_dirs:
			sourceEntries = self.add(doc, config, 'sourceEntries')
			for i in source_dirs:
				 self.add(doc, sourceEntries, 'entry',
							{'excluding': i,
							'flags': 'VALUE_WORKSPACE_PATH|RESOLVED',
							'kind': 'sourcePath',
							'name': ''})
				 self.add(doc, sourceEntries, 'entry',
							{
							'flags': 'VALUE_WORKSPACE_PATH|RESOLVED',
							'kind': 'sourcePath',
							'name': i})

		storageModule = self.add(doc, cconf, 'storageModule',
							{'moduleId': cdt_mk + '.buildtargets'})
		buildTargets = self.add(doc, storageModule, 'buildTargets')
		def addTargetWrap(name, runAll):
			return self.addTarget(doc, buildTargets, executable, name,
								'"%s" %s'%(waf, name), runAll)
		addTargetWrap('configure', True)
		addTargetWrap('dist', False)
		addTargetWrap('install', False)
		addTargetWrap('check', False)

		storageModule = self.add(doc, cproject, 'storageModule',
							{'moduleId': 'cdtBuildSystem',
							 'version': '4.0.0'})

		project = self.add(doc, storageModule, 'project',
					{'id': '%s.null.1'%appname, 'name': appname})

		doc.appendChild(cproject)
		return doc

	def impl_create_pydevproject(self, appname, system_path, user_path):
		# create a pydevproject file
		doc = Document()
		doc.appendChild(doc.createProcessingInstruction('eclipse-pydev', 'version="1.0"'))
		pydevproject = doc.createElement('pydev_project')
		prop = self.add(doc, pydevproject,
					   'pydev_property',
					   'python %d.%d'%(sys.version_info[0], sys.version_info[1]))
		prop.setAttribute('name', 'org.python.pydev.PYTHON_PROJECT_VERSION')
		prop = self.add(doc, pydevproject, 'pydev_property', 'Default')
		prop.setAttribute('name', 'org.python.pydev.PYTHON_PROJECT_INTERPRETER')
		# add waf's paths
		wafadmin = [p for p in system_path if p.find('wafadmin') != -1]
		if wafadmin:
			prop = self.add(doc, pydevproject, 'pydev_pathproperty',
					{'name':'org.python.pydev.PROJECT_EXTERNAL_SOURCE_PATH'})
			for i in wafadmin:
				self.add(doc, prop, 'path', i)
		if user_path:
			prop = self.add(doc, pydevproject, 'pydev_pathproperty',
					{'name':'org.python.pydev.PROJECT_SOURCE_PATH'})
			for i in user_path:
				self.add(doc, prop, 'path', '/'+appname+'/'+i)

		doc.appendChild(pydevproject)
		return doc

	def addDictionary(self, doc, parent, k, v):
		dictionary = self.add(doc, parent, 'dictionary')
		self.add(doc, dictionary, 'key', k)
		self.add(doc, dictionary, 'value', v)
		return dictionary

	def addTarget(self, doc, buildTargets, executable, name, buildTarget, runAllBuilders=True):
		target = self.add(doc, buildTargets, 'target',
						{'name': name,
						 'path': '',
						 'targetID': oe_cdt + '.build.MakeTargetBuilder'})
		self.add(doc, target, 'buildCommand', executable)
		self.add(doc, target, 'buildArguments', None)
		self.add(doc, target, 'buildTarget', buildTarget)
		self.add(doc, target, 'stopOnError', 'true')
		self.add(doc, target, 'useDefaultCommand', 'false')
		self.add(doc, target, 'runAllBuilders', str(runAllBuilders).lower())

	def add(self, doc, parent, tag, value = None):
		el = doc.createElement(tag)
		if (value):
			if type(value) == type(str()):
				el.appendChild(doc.createTextNode(value))
			elif type(value) == type(dict()):
				self.setAttributes(el, value)
		parent.appendChild(el)
		return el

	def setAttributes(self, node, attrs):
		for k, v in attrs.items():
			node.setAttribute(k, v)

