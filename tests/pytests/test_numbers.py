# -*- coding: utf-8 -*-

import unittest
from includes import *
from common import getConnectionByEnv, waitForIndex, sortedResults, toSortedFlatList
from time import sleep
from RLTest import Env
import math

def testCompression(env):
	accuracy = 0.000001
	repeat = int(math.sqrt(1 / accuracy))

	conn = getConnectionByEnv(env)
	pl = conn.pipeline()
	env.cmd('ft.create', 'idx', 'SCHEMA', 'n', 'numeric')
	for i in range(repeat):
		value = accuracy * i
		pl.execute_command('hset', i, 'n', str(value))
		if (i % 999) is 0:
			pl.execute()
	pl.execute()

	for i in range(repeat):
		value = accuracy * i
		env.expect('ft.search', 'idx', ('@n:[%s %s]' % (value, value))).equal([1L, str(i), ['n', str(value)]])
  
def testSanity(env):
	env.skipOnCluster()
	repeat = 100000
	conn = getConnectionByEnv(env)
	env.cmd('ft.create', 'idx', 'SCHEMA', 'n', 'numeric')
	for i in range(repeat):
		conn.execute_command('hset', i, 'n', i % 100)
	env.expect('ft.search', 'idx', ('@n:[0 %d]' % (repeat)), 'limit', 0 ,0).equal([repeat])
	env.expect('FT.DEBUG', 'numidx_summary', 'idx', 'n') \
				.equal(['numRanges', 12L, 'numEntries', 100000L, 'lastDocId', 100000L, 'revisionId', 11L])

def testCompressionConfig(env):
	env.skipOnCluster()
	conn = getConnectionByEnv(env)
	env.cmd('ft.create', 'idx', 'SCHEMA', 'n', 'numeric')

	# w/o compression. exact number match.
	env.expect('ft.config', 'set', '_NUMERIC_COMPRESS', 'false').equal('OK')
	for i in range(100):
	  env.execute_command('hset', i, 'n', str(1 + i / 100.0))
	for i in range(100):
		num = str(1 + i / 100.0)
		env.expect('ft.search', 'idx', '@n:[%s %s]' % (num, num)).equal([1L, str(i), ['n', num]])

	# with compression. no exact number match.
	env.expect('ft.config', 'set', '_NUMERIC_COMPRESS', 'true').equal('OK')
	for i in range(100):
	  env.execute_command('hset', i, 'n', str(1 + i / 100.0))
	
	# delete keys where compression does not change value
	env.execute_command('del', '0')
	env.execute_command('del', '25')
	env.execute_command('del', '50')
	env.execute_command('del', '75')

	for i in range(100):
		num = str(1 + i / 100.0)
		env.expect('ft.search', 'idx', '@n:[%s %s]' % (num, num)).equal([0L])

def testRangeParentsConfig(env):
	env.skipOnCluster()
	elements = 1000
	conn = getConnectionByEnv(env)

	concurrent = env.cmd('ft.config', 'get', 'CONCURRENT_WRITE_MODE')
	if str(concurrent[0][1]) == 'true':
		env.skip()

	result = [['numRanges', 6L], ['numRanges', 8L]]
	for test in range(2):
		# check number of ranges
		env.cmd('ft.create', 'idx0', 'SCHEMA', 'n', 'numeric')
		for i in range(elements):
			env.execute_command('hset', i, 'n', i)
		actual_res = env.cmd('FT.DEBUG', 'numidx_summary', 'idx0', 'n')
		env.assertEqual(actual_res[0:2], result[test])

		# reset with old ranges parents param
		env.cmd('ft.drop', 'idx0')
		env.expect('ft.config', 'set', '_NUMERIC_RANGES_PARENTS', '2').equal('OK')

def testFirst(env):
	env.skipOnCluster()
	env.cmd('FT.CONFIG', 'SET', '_PRINT_PROFILE_CLOCK', 'false')
	repeat = 1000
	conn = getConnectionByEnv(env)
	for i in range(repeat):
		conn.execute_command('HSET', i, 'n', i)
	env.cmd('FT.CREATE', 'idx', 'SCHEMA', 'n', 'NUMERIC', 'SORTABLE')
	waitForIndex(env, 'idx')

	res = env.cmd('FT.SEARCH', 'idx', '@n:[0 inf]', 'SORTBY', 'n', 'LIMIT', 0 ,10, 'FIRST', 'NOCONTENT')
	env.assertEqual(res[1:], ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9'])
	
	# FT.PROFILE with 'FIRST'
	res = env.cmd('FT.PROFILE', 'idx', 'SEARCH', 'QUERY', '@n:[0 inf]', 'SORTBY', 'n', 'LIMIT', 0 ,10, 'FIRST', 'NOCONTENT')
	env.assertEqual(len(res[1][3]), 2) # title + single iterator since ram=nge contains the first iterator

	res = env.cmd('FT.SEARCH', 'idx', '@n:[100 inf]', 'SORTBY', 'n', 'LIMIT', 0 ,10, 'FIRST', 'NOCONTENT')
	env.assertEqual(res[1:], ['100', '101', '102', '103', '104', '105', '106', '107', '108', '109'])
