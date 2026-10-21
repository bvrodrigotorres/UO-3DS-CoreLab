#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import importlib.util
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('export_scene', ROOT/'tools/export_scene.py')
exporter = importlib.util.module_from_spec(spec)
spec.loader.exec_module(exporter)


class SceneTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.binary = Path(cls.temp.name)/'core_tests'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g', '-I'+str(ROOT/'include'), '-I'+str(ROOT/'tests'), str(ROOT/'tests/core_tests.cpp'), str(ROOT/'tests/reference.cpp'), str(ROOT/'source/core.cpp'), str(ROOT/'source/scene_io.cpp'), str(ROOT/'source/exploration.cpp'), str(ROOT/'source/avatar_io.cpp'), '-o', str(cls.binary)], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def test_upstream_differential_and_geometry(self):
        subprocess.run([str(self.binary)], check=True)

    def test_exterior_visibility_and_hud(self):
        binary=Path(self.temp.name)/'presentation_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/presentation_tests.cpp'),str(ROOT/'source/hud.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

    def fixture(self):
        return exporter.HEAD.pack(b'UCL1',1,2,2,0,0,0,0,0,0,0,0)+bytes(9)+exporter.LAND.pack(3,65535,0,0)*4

    def load(self, raw):
        p = Path(self.temp.name)/'scene.bin'
        p.write_bytes(raw)
        return subprocess.run([str(self.binary), str(p)], capture_output=True, text=True).returncode

    def test_valid_scene(self):
        self.assertEqual(self.load(self.fixture()),0)

    def test_save_map_and_avatar_provenance(self):
        with tempfile.TemporaryDirectory() as folder:
            subprocess.run([str(self.binary),'--exploration',folder],check=True)

    def test_avatar_export_default_is_unarmed_run(self):
        import ast
        tree=ast.parse((ROOT/'tools/export_avatar.py').read_text())
        calls=[n for n in ast.walk(tree) if isinstance(n,ast.Call) and n.args and isinstance(n.args[0],ast.Constant) and n.args[0].value=='--run-action']
        self.assertEqual(len(calls),1)
        self.assertEqual(next(k.value.value for k in calls[0].keywords if k.arg=='default'),2)

    def test_truncations_and_trailing_bytes(self):
        raw = self.fixture()
        for size in (0,4,20,31,32,len(raw)-1):
            self.assertEqual(self.load(raw[:size]),2)
        self.assertEqual(self.load(raw+b'x'),2)

    def test_bad_dimensions_spawn_and_image_index(self):
        for offset, value in ((6,33),(8,0),(24,2)):
            raw = bytearray(self.fixture());struct.pack_into('<H',raw,offset,value)
            self.assertEqual(self.load(raw),2)
        raw = bytearray(self.fixture());struct.pack_into('<H',raw,32+9+2,0)
        self.assertEqual(self.load(raw),2)

    def test_patch_duplicates_last_entry_wins(self):
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)
            (p/'mapdifl0.mul').write_bytes(struct.pack('<II',7,7))
            (p/'mapdif0.mul').write_bytes(bytes(196)+b'a'*196)
            self.assertEqual(exporter.differences(p,'mapdif'),{7:b'a'*196})

    def test_malformed_static_patch_rejected(self):
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)
            (p/'stadifl0.mul').write_bytes(struct.pack('<I',7))
            (p/'stadifi0.mul').write_bytes(struct.pack('<III',0,8,0))
            (p/'stadif0.mul').write_bytes(bytes(8))
            with self.assertRaises(ValueError):
                exporter.differences(p,'stadif','stadifi0.mul')


if __name__=='__main__':
    unittest.main(verbosity=2)
