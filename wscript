# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('ccncaching', ['core'])
    module.source = [
        'model/ccncaching.cc',
		'model/ccncaching.cc',
		'model/CcnModule.cc',
		'model/CCN_Name.cc',
		'model/ccn-packets.cc',
		'helper/experiment_globals.cc',
		'helper/Graph.cc',
		'model/Initializer.cc',
		'model/local_app.cc',
		'model/md5.cc',
		'model/city.cc',
		'helper/Parser.cc',
		'model/PIT.cc',
		'model/PIT_Key.cc',
		'model/PTuple.cc',
		'model/Receiver.cc',
		'model/ResultPrinter.cc',
		'model/Sender.cc',
		'model/Cache.cc',
		'model/sha1.cc',
		'model/Trie.cc',
		'model/TrieNode.cc',
		'helper/BootstrappingHelper.cc',
		
        ]
    #module.cxxflags = ['-std=c++11']
    #module.linkflags = ['-lbf', '-lpthread']
    module_test = bld.create_ns3_module_test_library('ccncaching')
    module_test.source = [
        'test/ccncaching-test-suite.cc',
        ]

    headers = bld(features='ns3header')
    headers.module = 'ccncaching'
    headers.source = [
        'model/ccncaching.h',
       	'model/ccncaching.h',
		'model/CcnModule.h',
		'model/CCN_Name.h',
		'model/ccn-packets.h',
		'helper/experiment_globals.h',
		'helper/Graph.h',
		'model/Initializer.h',
		'model/local_app.h',
		'model/md5.h',
		'model/city.h',
		'model/citycrc.h',
		'model/config.h',
		'helper/Parser.h',
		'model/PIT.h',
		'model/PIT_Key.h',
		'model/PTuple.h',
		'model/Receiver.h',
		'model/ResultPrinter.h',
		'model/Sender.h',
		'model/Cache.h',
		'model/sha1.h',
		'model/Trie.h',
		'model/TrieNode.h',
		'model/utils.h',
		#'helper/helper.h',
		'helper/BootstrappingHelper.h',
        ]

    #headers.cxxflags = ['-std=c++11']
    #headers.linkflags = ['-lbf', '-lpthread']
    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()

