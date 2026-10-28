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
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/presentation_tests.cpp'),str(ROOT/'source/hud.cpp'),str(ROOT/'source/character.cpp'),str(ROOT/'source/inventory.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/exploration.cpp'),'-o',str(binary)],check=True)
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
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/inventory_tests.cpp'),str(ROOT/'source/inventory.cpp'),str(ROOT/'source/hud.cpp'),str(ROOT/'source/character.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/scene_io.cpp'),str(ROOT/'source/exploration.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary),self.temp.name],check=True)

    def test_equipment(self):
        binary=Path(self.temp.name)/'equipment_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/equipment_tests.cpp'),str(ROOT/'source/inventory.cpp'),str(ROOT/'source/equipment.cpp'),str(ROOT/'source/hud.cpp'),str(ROOT/'source/character.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/exploration.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary),self.temp.name],check=True)

    def test_packed_texture_reads(self):
        binary=Path(self.temp.name)/'packed_textures_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-pthread','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/packed_textures_tests.cpp'),str(ROOT/'source/save_queue.cpp'),'-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_expedition_npc_pilot(self):
        binary=Path(self.temp.name)/'expedition_npcs_tests'
        sources=['tests/expedition_npcs_tests.cpp','source/combat.cpp','source/npcs.cpp','source/inventory.cpp','source/core.cpp','source/exploration.cpp','source/doors.cpp','source/character.cpp']
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include')]+[str(ROOT/f) for f in sources]+['-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_realm49_memory_and_aquatic_movement(self):
        binary=Path(self.temp.name)/'realm49_tests'
        sources=['tests/realm49_tests.cpp','source/combat.cpp','source/npcs.cpp','source/inventory.cpp','source/hud.cpp','source/character.cpp','source/core.cpp','source/exploration.cpp','source/doors.cpp']
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include')]+[str(ROOT/f) for f in sources]+['-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_startup_memory_budget(self):
        binary=Path(self.temp.name)/'startup_memory_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/startup_memory_tests.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/scene_io.cpp'),'-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_delayed_bandages(self):
        binary=Path(self.temp.name)/'bandage_tests'
        sources=['tests/bandage_tests.cpp','source/combat.cpp','source/npcs.cpp','source/inventory.cpp','source/core.cpp','source/exploration.cpp','source/doors.cpp','source/character.cpp']
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-g','-I'+str(ROOT/'include')]+[str(ROOT/f) for f in sources]+['-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_texture_previews(self):
        binary=Path(self.temp.name)/'texture_preview_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/texture_preview_tests.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

    def test_serialized_platform_io(self):
        binary=Path(self.temp.name)/'platform_io_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-pthread','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/platform_io_tests.cpp'),str(ROOT/'source/platform_io.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

    def test_background_world_stream(self):
        binary=Path(self.temp.name)/'world_stream_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-pthread','-I'+str(ROOT/'include'),str(ROOT/'tests/world_stream_tests.cpp'),str(ROOT/'source/world_stream.cpp'),str(ROOT/'source/world_expedition.cpp'),str(ROOT/'source/world_archive.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/save_queue.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

    def test_expedition_transition_memory_and_recovery(self):
        binary=Path(self.temp.name)/'expedition_transition_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/expedition_transition_tests.cpp'),str(ROOT/'source/world_expedition.cpp'),str(ROOT/'source/world_archive.cpp'),str(ROOT/'source/core.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

    def test_world_expedition(self):
        binary=Path(self.temp.name)/'world_expedition_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/world_expedition_tests.cpp'),str(ROOT/'source/world_expedition.cpp'),str(ROOT/'source/world_archive.cpp'),str(ROOT/'source/core.cpp'),'-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_full_world_archive(self):
        binary=Path(self.temp.name)/'world_archive_tests'
        sources=['tests/world_archive_tests.cpp','source/world_archive.cpp','source/core.cpp']
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include')]+[str(ROOT/f) for f in sources]+['-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_quests_and_conversations(self):
        binary=Path(self.temp.name)/'quest43_tests'
        sources=['tests/quest43_tests.cpp','source/quests.cpp','source/combat.cpp','source/npcs.cpp','source/inventory.cpp','source/hud.cpp','source/character.cpp','source/core.cpp','source/scene_io.cpp','source/exploration.cpp','source/doors.cpp']
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include')]+[str(ROOT/f) for f in sources]+['-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_startup_and_campaigns(self):
        binary=Path(self.temp.name)/'startup_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/startup_tests.cpp'),str(ROOT/'source/startup.cpp'),str(ROOT/'source/inventory.cpp'),str(ROOT/'source/hud.cpp'),str(ROOT/'source/character.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/exploration.cpp'),'-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:
            subprocess.run([str(binary),folder],check=True)

    def test_character_customization(self):
        binary=Path(self.temp.name)/'character_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/character_tests.cpp'),str(ROOT/'source/character.cpp'),str(ROOT/'source/hud.cpp'),str(ROOT/'source/inventory.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/exploration.cpp'),'-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:
            subprocess.run([str(binary),folder],check=True)

    def test_real_wardrobe(self):
        binary=Path(self.temp.name)/'wardrobe_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/wardrobe_tests.cpp'),str(ROOT/'source/character.cpp'),str(ROOT/'source/inventory.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/exploration.cpp'),'-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:
            subprocess.run([str(binary),folder],check=True)

    def test_customization24_and_quick_equipment(self):
        binary=Path(self.temp.name)/'customization24_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/customization24_tests.cpp'),str(ROOT/'source/character.cpp'),str(ROOT/'source/inventory.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/exploration.cpp'),'-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:
            subprocess.run([str(binary),folder],check=True)

    def test_owned_wardrobe_and_hat_preview(self):
        binary=Path(self.temp.name)/'wardrobe26_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/wardrobe26_tests.cpp'),str(ROOT/'source/character.cpp'),str(ROOT/'source/inventory.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/exploration.cpp'),'-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:
            subprocess.run([str(binary),folder],check=True)

    def test_save_schedule_and_equipment_cache(self):
        binary=Path(self.temp.name)/'performance_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/performance_tests.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

    def test_auto_defense_and_healing(self):
        binary=Path(self.temp.name)/'combat34_tests'
        sources=['tests/combat34_tests.cpp','source/combat.cpp','source/npcs.cpp','source/inventory.cpp','source/core.cpp','source/exploration.cpp','source/doors.cpp']
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include')]+[str(ROOT/f) for f in sources]+['-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_controls_healing_and_pause_save(self):
        binary=Path(self.temp.name)/'controls35_tests'
        sources=['tests/controls35_tests.cpp','source/combat.cpp','source/npcs.cpp','source/inventory.cpp','source/core.cpp','source/exploration.cpp','source/doors.cpp']
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include')]+[str(ROOT/f) for f in sources]+['-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_skills_progression_and_persistence(self):
        binary=Path(self.temp.name)/'skills36_tests'
        sources=['tests/skills36_tests.cpp','source/combat.cpp','source/npcs.cpp','source/inventory.cpp','source/core.cpp','source/exploration.cpp','source/doors.cpp','source/hud.cpp','source/character.cpp','source/startup.cpp']
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include')]+[str(ROOT/f) for f in sources]+['-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_miss_and_healing_overhead_feedback(self):
        binary=Path(self.temp.name)/'combat37_tests'
        sources=['tests/combat37_tests.cpp','source/combat.cpp','source/npcs.cpp','source/inventory.cpp','source/core.cpp','source/exploration.cpp','source/doors.cpp']
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include')]+[str(ROOT/f) for f in sources]+['-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_mount_orientation_and_radar(self):
        binary=Path(self.temp.name)/'radar40_tests'
        sources=['tests/radar40_tests.cpp','source/hud.cpp','source/character.cpp','source/inventory.cpp','source/core.cpp','source/exploration.cpp','source/npcs.cpp','source/doors.cpp']
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include')]+[str(ROOT/f) for f in sources]+['-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

    def test_mount_and_instant_pause(self):
        binary=Path(self.temp.name)/'mount39_tests'
        sources=['tests/mount39_tests.cpp','source/save_queue.cpp','source/combat.cpp','source/npcs.cpp','source/inventory.cpp','source/core.cpp','source/exploration.cpp','source/doors.cpp','source/hud.cpp','source/character.cpp','source/startup.cpp']
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-pthread','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include')]+[str(ROOT/f) for f in sources]+['-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_slow_storage_terrain_and_animation_progress(self):
        binary=Path(self.temp.name)/'streaming412_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-pthread','-I'+str(ROOT/'include'),str(ROOT/'tests/streaming412_tests.cpp'),str(ROOT/'source/save_queue.cpp'),'-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_npc_pose_streaming_and_resource_leases(self):
        binary=Path(self.temp.name)/'streaming41_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-pthread','-I'+str(ROOT/'include'),str(ROOT/'tests/streaming41_tests.cpp'),str(ROOT/'source/save_queue.cpp'),'-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_old3ds_io_and_runtime_performance(self):
        binary=Path(self.temp.name)/'performance38_tests'
        sources=['tests/performance38_tests.cpp','source/save_queue.cpp','source/combat.cpp','source/npcs.cpp','source/inventory.cpp','source/core.cpp','source/exploration.cpp','source/doors.cpp','source/hud.cpp','source/character.cpp']
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-pthread','-I'+str(ROOT/'include')]+[str(ROOT/f) for f in sources]+['-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_developer_teleport_and_stack_keys(self):
        binary=Path(self.temp.name)/'developer33_tests'
        sources=['tests/developer33_tests.cpp','source/npcs.cpp','source/inventory.cpp','source/core.cpp','source/scene_io.cpp','source/exploration.cpp','source/doors.cpp','source/character.cpp','source/hud.cpp']
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include')]+[str(ROOT/f) for f in sources]+['-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

    def test_drag_and_ground_inventory(self):
        binary=Path(self.temp.name)/'inventory32_tests'
        sources=['tests/inventory32_tests.cpp','source/combat.cpp','source/npcs.cpp','source/inventory.cpp','source/core.cpp','source/exploration.cpp','source/doors.cpp','source/character.cpp','source/hud.cpp']
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include')]+[str(ROOT/f) for f in sources]+['-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_pause_equipment_and_jewelry(self):
        binary=Path(self.temp.name)/'equipment31_tests'
        sources=['tests/equipment31_tests.cpp','source/inventory.cpp','source/core.cpp','source/exploration.cpp','source/character.cpp','source/hud.cpp']
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include')]+[str(ROOT/f) for f in sources]+['-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_economy_and_gear(self):
        binary=Path(self.temp.name)/'economy30_tests'
        sources=['tests/economy30_tests.cpp','source/commerce.cpp','source/combat.cpp','source/npcs.cpp','source/inventory.cpp','source/core.cpp','source/exploration.cpp','source/doors.cpp','source/character.cpp','source/hud.cpp']
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include')]+[str(ROOT/f) for f in sources]+['-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_large_region(self):
        binary=Path(self.temp.name)/'large_world_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/large_world_tests.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/scene_io.cpp'),str(ROOT/'source/doors.cpp'),str(ROOT/'source/inventory.cpp'),str(ROOT/'source/exploration.cpp'),str(ROOT/'source/journal.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

    def test_expanded_region_and_import(self):
        binary=Path(self.temp.name)/'region29_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/region29_tests.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/scene_io.cpp'),str(ROOT/'source/doors.cpp'),str(ROOT/'source/inventory.cpp'),str(ROOT/'source/exploration.cpp'),str(ROOT/'source/journal.cpp'),str(ROOT/'source/region_migration.cpp'),str(ROOT/'source/startup.cpp'),str(ROOT/'source/character.cpp'),str(ROOT/'source/combat.cpp'),str(ROOT/'source/npcs.cpp'),'-o',str(binary)],check=True)

        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_character_pixel_depth(self):
        binary=Path(self.temp.name)/'character_depth_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/character_depth_tests.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/scene_io.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

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

    def test_original_door_precedence(self):
        import sys
        sys.path.insert(0,str(ROOT/'tools'))
        from import_britain import resolve_doors
        source='class MetalDoor : BaseDoor { : base(0x675 + (2 * (int)facing), 0x676 + (2 * (int)facing), 0, 0, offset) {} }'
        explicit=[dict(x=3,y=3,z=0,art=0x675,hue=0,type='MetalDoor',props='Facing=WestCW'),dict(x=4,y=3,z=0,art=0x677,hue=0,type='MetalDoor',props='Facing=EastCCW')]
        generic=[dict(x=3,y=3,z=0,art=0x6A5,hue=0,type='DarkWoodDoor',open_art=0x6A6,dx=-1,dy=1,facing='WestCW',pair=None)]
        remaining,doors,promoted=resolve_doors([(7,7,0,0x675,0)],explicit,generic,source,16)
        self.assertEqual(remaining,[])
        self.assertEqual(len(doors),3)
        self.assertEqual(doors[0]['art'],0x675)
        self.assertEqual(doors[0]['open_art'],0x676)
        self.assertEqual(doors[0]['pair'],1)
        self.assertEqual(doors[1]['pair'],0)
        self.assertEqual(promoted,[0])
        self.assertEqual(doors[2]['base_index'],0)

    def test_door_only_save_migration(self):
        import sys
        sys.path.insert(0,str(ROOT/'tools'))
        from export_save_compat import export,fnv
        with tempfile.TemporaryDirectory() as folder:
            old=Path(folder)/'old.bin';new=Path(folder)/'new.bin'
            raw=self.fixture();old.write_bytes(raw);new.write_bytes(raw)
            alias=export(old,new)
            magic,checksum,pos,inventory=struct.unpack('<4sIII',alias.read_bytes())
            self.assertEqual(magic,b'UWC1');self.assertEqual(checksum,fnv(raw));self.assertTrue(pos and inventory)
            self.assertEqual(subprocess.run([str(self.binary),str(new)],capture_output=True).returncode,0)
            data=bytearray(alias.read_bytes());data[4]^=1;alias.write_bytes(data)
            self.assertEqual(subprocess.run([str(self.binary),str(new)],capture_output=True).returncode,2)
            new.write_bytes(raw[:32]+bytes([1])+raw[33:])
            with self.assertRaises(ValueError):export(old,new)

    def test_creature_constructor_rules(self):
        import sys
        sys.path.insert(0,str(ROOT/'tools'))
        from export_population import creature_definition
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            (root/'Cow.cs').write_text('class Cow : BaseCreature { Cow() : base(AIType.AI_Animal, FightMode.Aggressor, 10) { Body = Utility.RandomList(216,231); SetHits(18); SetDamage(1,4); if (rare) FightMode = FightMode.Closest; } }')
            cow=creature_definition('Cow',root)
            self.assertFalse(cow['aggressive'])
            self.assertEqual((cow['body'],cow['hits'],cow['damage']),(216,18,[1,4]))
            (root/'Orc.cs').write_text('class Orc : BaseCreature { Orc() : base(AIType.AI_Melee, FightMode.Closest, 10) { Body = 17; SetHits(58,72); SetDamage(5,7); } public override bool DisallowAllMoves { get { return true; } } }')
            orc=creature_definition('orc',root)
            self.assertTrue(orc['aggressive'] and orc['stationary'])
            self.assertEqual((orc['body'],orc['hits']),(17,65))
            (root/'Horse.cs').write_text('class Horse : BaseMount { Horse(string name) : base(name, 0xE2, 0x3EA0, AIType.AI_Animal, FightMode.Aggressor, 10) { Body = m_IDs[random*2]; SetHits(28,45); } }')
            horse=creature_definition('Horse',root)
            self.assertEqual(horse['body'],226)
            self.assertFalse(horse['aggressive'])

    def test_spawner_identity_and_rectangles(self):
        import sys
        sys.path.insert(0,str(ROOT/'tools'))
        from export_population import spawn_requests
        def point(uid,x,y,width,height,objects,count,running='True'):
            values=dict(UniqueId=uid,Map='Felucca',IsRunning=running,InContainer='False',X=x,Y=y,Width=width,Height=height,CentreX=x+width//2,CentreY=y+height//2,CentreZ=0,Range=5,Objects2=objects,MaxCount=count)
            return '<Points>'+''.join('<'+k+'>'+str(v)+'</'+k+'>' for k,v in values.items())+'</Points>'
        one=point('a',10,10,0,0,'mage:MX=2:OBJ=alchemist:MX=2',3)
        two=point('b',10,10,0,0,'mage:MX=2:OBJ=alchemist:MX=2',3)
        edge=point('c',0,20,12,4,'Chicken:MX=5',5)
        inactive=point('d',11,11,0,0,'Cat:MX=2',2,'False')
        with tempfile.TemporaryDirectory() as t:
            path=Path(t)/'spawns.xml';path.write_text('<Spawns>'+one+two+edge+inactive+'</Spawns>')
            rows,excluded,duplicates=spawn_requests(path,(10,10),16)
            self.assertEqual(len(rows),11)
            self.assertEqual(sum(r['animal'] for r in rows),5)
            self.assertEqual(duplicates,0)
            self.assertEqual(set(r['spawner'] for r in rows),{'a','b','c'})
            self.assertEqual(next(r['bounds'] for r in rows if r['animal']),[0,10,2,14])
            path.write_text('<Spawns>'+one+one+'</Spawns>')
            rows,_,duplicates=spawn_requests(path,(10,10),16)
            self.assertEqual((len(rows),duplicates),(3,1))

    def test_spawn_placement_prefers_source_floor(self):
        binary=Path(self.temp.name)/'place_population'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I'+str(ROOT/'include'),str(ROOT/'tools/place_population.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/scene_io.cpp'),str(ROOT/'source/npcs.cpp'),str(ROOT/'source/doors.cpp'),'-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);scene=root/'place.bin';request=root/'request.bin';placed=root/'placed.bin'
            wall=exporter.OBJECT.pack(1,1,0,20,4,0,65535,0x40,0,0)
            roof=exporter.OBJECT.pack(1,1,60,0,5,0,65535,0x200,1,0)
            raw=bytearray(self.fixture());struct.pack_into('<I',raw,12,2)
            scene.write_bytes(raw+wall+roof)
            request.write_bytes(struct.pack('<HHhHHH32s4H',1,1,0,0,3,0,b'MAGE',1,1,1,1))
            subprocess.run([str(binary),str(scene),str(request),str(placed)],check=True)
            x,y,z=struct.unpack_from('<HHh',placed.read_bytes())
            self.assertEqual(z,0)
            self.assertNotEqual((x,y),(1,1))

    def test_roof_pause_and_overhead_names(self):
        import sys
        sys.path.insert(0,str(ROOT/'tools'))
        from export_journal import fnv
        binary=Path(self.temp.name)/'presentation_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/overhead_pause_roof_tests.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/scene_io.cpp'),str(ROOT/'source/npcs.cpp'),str(ROOT/'source/doors.cpp'),'-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);scene=root/'scene.bin';population=root/'population.bin';names=root/'names.bin'
            raw=self.fixture();scene.write_bytes(raw)
            body=struct.pack('<hhHHII',4,7,5,7,64,1234)+struct.pack('<HHhHHH32s',1,1,0,0,3,0,b'MAGE')
            pop=struct.pack('<4sHHII',b'UNP1',1,1,fnv(raw),fnv(body))+body;population.write_bytes(pop)
            label=struct.pack('<32s48sH3B',b'Alice',b'the mage',0x35,238,238,0)
            names.write_bytes(struct.pack('<4sHHIII',b'UNL1',1,0,fnv(raw),fnv(pop),fnv(label))+label)
            subprocess.run([str(binary),str(scene),str(population),str(names)],check=True)

    def test_npc_names_source_properties(self):
        import sys
        sys.path.insert(0,str(ROOT/'tools'))
        from export_npc_names import describe
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            (root/'BaseVendor.cs').write_text('class BaseVendor : BaseCreature { public override bool IsInvulnerable { get { return true; } } }')
            (root/'Mage.cs').write_text('class Mage : BaseVendor { Mage() : base("the mage") { packKey.Name = "a ship key"; } }')
            self.assertEqual(describe('Mage',root),(None,'the mage',True))
            (root/'Healer.cs').write_text('class Healer : BaseVendor { public override bool IsInvulnerable { get { return false; } } Title = "the healer"; }')
            self.assertEqual(describe('Healer',root),(None,'the healer',False))

    def test_name_legibility_and_proximity(self):
        import sys
        sys.path.insert(0,str(ROOT/'tools'))
        from export_overhead_font import export
        raw=bytearray()
        for font in range(10):
            raw+=b'\0'
            for char in range(32,256):
                if font==9 and char==65:raw+=bytes([3,3,0])+struct.pack('<9H',0,0x7fff,0,0x7fff,0,0x7fff,0x7fff,0x7fff,0x7fff)
                else:raw+=bytes(3)
        data=export(raw)
        with self.assertRaises(ValueError):export(raw[:-2])
        binary=Path(self.temp.name)/'name_legibility_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/name_legibility_tests.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/scene_io.cpp'),str(ROOT/'source/npcs.cpp'),str(ROOT/'source/doors.cpp'),'-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:
            font=Path(folder)/'npc_font.bin';font.write_bytes(data);subprocess.run([str(binary),str(font)],check=True)

    def test_headgear_covers_hair(self):
        import sys
        sys.path.insert(0,str(ROOT/'tools'))
        from export_population import composition_layers
        animation=lambda art:100+art
        layer=lambda art:6 if art==2 else 5
        self.assertEqual(composition_layers(400,0,[(1,11),(2,22),(3,33)],701,44,animation,layer),[(400,0),(101,11),(103,33),(701,44),(102,22)])
        self.assertEqual(composition_layers(400,0,[(1,11)],701,44,animation,layer),[(400,0),(101,11),(701,44)])

    def test_npc_push_and_status(self):
        binary=Path(self.temp.name)/'npc_push_status_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/npc_push_status_tests.cpp'),str(ROOT/'source/hud.cpp'),str(ROOT/'source/character.cpp'),str(ROOT/'source/inventory.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/scene_io.cpp'),str(ROOT/'source/exploration.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

    def test_combat_and_corpses(self):
        binary=Path(self.temp.name)/'combat_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/combat_tests.cpp'),str(ROOT/'source/combat.cpp'),str(ROOT/'source/npcs.cpp'),str(ROOT/'source/doors.cpp'),str(ROOT/'source/hud.cpp'),str(ROOT/'source/character.cpp'),str(ROOT/'source/inventory.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/scene_io.cpp'),str(ROOT/'source/exploration.cpp'),'-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_combat_pursuit_and_swings(self):
        binary=Path(self.temp.name)/'combat27_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/combat27_tests.cpp'),str(ROOT/'source/combat.cpp'),str(ROOT/'source/npcs.cpp'),str(ROOT/'source/doors.cpp'),str(ROOT/'source/hud.cpp'),str(ROOT/'source/character.cpp'),str(ROOT/'source/inventory.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/scene_io.cpp'),str(ROOT/'source/exploration.cpp'),'-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_damage_blood_and_recovery(self):
        binary=Path(self.temp.name)/'combat28_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/combat28_tests.cpp'),str(ROOT/'source/combat.cpp'),str(ROOT/'source/npcs.cpp'),str(ROOT/'source/doors.cpp'),str(ROOT/'source/hud.cpp'),str(ROOT/'source/character.cpp'),str(ROOT/'source/inventory.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/scene_io.cpp'),str(ROOT/'source/exploration.cpp'),'-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

    def test_inventory_pages_and_take_all(self):
        binary=Path(self.temp.name)/'inventory_pages_tests'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-I'+str(ROOT/'include'),str(ROOT/'tests/inventory_pages_tests.cpp'),str(ROOT/'source/combat.cpp'),str(ROOT/'source/npcs.cpp'),str(ROOT/'source/doors.cpp'),str(ROOT/'source/hud.cpp'),str(ROOT/'source/character.cpp'),str(ROOT/'source/inventory.cpp'),str(ROOT/'source/core.cpp'),str(ROOT/'source/scene_io.cpp'),str(ROOT/'source/exploration.cpp'),'-o',str(binary)],check=True)
        with tempfile.TemporaryDirectory() as folder:subprocess.run([str(binary),folder],check=True)

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
        raw=exporter.HEAD.pack(b'UCL1',1,256,256,0,0,1408,1536,93,76,20,0)+bytes(257*257)+exporter.LAND.pack(3,65535,0,0)*(256*256)
        self.assertEqual(self.load(raw),0)

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
