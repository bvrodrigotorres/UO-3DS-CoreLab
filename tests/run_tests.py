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
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/presentation_tests.cpp'),str(ROOT/'source/hud.cpp'),str(ROOT/'source/inventory.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/exploration.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

    def test_journal_validation(self):
        binary=Path(self.temp.name)/'journal_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/journal_tests.cpp'),str(ROOT/'source/journal.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary),self.temp.name],check=True)

    def test_doors_and_collision(self):
        binary=Path(self.temp.name)/'door_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/door_tests.cpp'),str(ROOT/'source/doors.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/scene_io.cpp'),str(ROOT/'source/exploration.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary),self.temp.name],check=True)

    def test_inventory_transactions(self):
        binary=Path(self.temp.name)/'inventory_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/inventory_tests.cpp'),str(ROOT/'source/inventory.cpp'),str(ROOT/'source/hud.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/scene_io.cpp'),str(ROOT/'source/exploration.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary),self.temp.name],check=True)

    def test_equipment(self):
        binary=Path(self.temp.name)/'equipment_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/equipment_tests.cpp'),str(ROOT/'source/inventory.cpp'),str(ROOT/'source/equipment.cpp'),str(ROOT/'source/hud.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/exploration.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary),self.temp.name],check=True)

    def test_startup_and_campaigns(self):
        binary=Path(self.temp.name)/'startup_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/startup_tests.cpp'),str(ROOT/'source/startup.cpp'),str(ROOT/'source/inventory.cpp'),str(ROOT/'source/hud.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/exploration.cpp'),'-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:
            subprocess.run([str(binary),folder],check=True)

    def test_decoration_parser(self):
        import sys
        sys.path.insert(0,str(ROOT/'tools'))
        from import_britain import placements
        rows=placements('Static 0x04B7\n1501 1611 40\n# ignored\nDrawer 0x0A2C (Hue=0x25)\n1503 1610 40\n2000 2000 0\n',1488,1596,32)
        self.assertEqual(len(rows),2)
        self.assertEqual((rows[0]['x'],rows[0]['y'],rows[0]['art']),(13,15,1207))
        self.assertEqual(rows[1]['hue'],37)
        with self.assertRaises(ValueError):placements('SomeAddon 0x123\n1501 1611 40\n',1488,1596,32)
        with self.assertRaises(ValueError):placements('malformed',1488,1596,32)

    def test_npc_simulation(self):
        binary=Path(self.temp.name)/'npc_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/npc_tests.cpp'),str(ROOT/'source/npcs.cpp'),str(ROOT/'source/doors.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/scene_io.cpp'),'-o',str(binary)],check=True)
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
