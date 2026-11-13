# SPDX-License-Identifier: Apache-2.0
"""Guard against false-green results in the integration runner."""
from pathlib import Path
import tempfile
import unittest
import xml.etree.ElementTree as ET

from run_tests import hbm_input, memory_isolation, parallelism, validate


class EvidenceChecks(unittest.TestCase):
    def test_shared_backend_cannot_pass_as_two_memories(self):
        log = ("Ramulator2 successfully initialized for interface 3\n"
               "[MDPI-MEMORY] ID=3,CYCLES=100,DRAM_TICKS=300,READS=40,COMPLETED=40,WRITES=20\n")
        with self.assertRaisesRegex(ValueError, "2 independent active memories"):
            memory_isolation(log, 2)

    def test_memory_clock_is_not_multiplied_by_port_count(self):
        first = ("Ramulator2 successfully initialized for interface 3\n"
                 "[MDPI-MEMORY] ID=3,CYCLES=100,DRAM_TICKS=300,READS=20,COMPLETED=20,WRITES=0\n")
        second = ("Ramulator2 successfully initialized for interface 4\n"
                  "[MDPI-MEMORY] ID=4,CYCLES=100,DRAM_TICKS=300,READS=20,COMPLETED=20,WRITES=20\n")
        self.assertEqual(len(memory_isolation(first + second, 2)), 2)
        with self.assertRaisesRegex(ValueError, "simulated accelerator cycles"):
            memory_isolation(first + second, 2, 50)
        write_only = second.replace("READS=20,COMPLETED=20", "READS=0,COMPLETED=0")
        self.assertEqual(len(memory_isolation(first + write_only, 2)), 2)
        with self.assertRaisesRegex(ValueError, "clock advanced incorrectly"):
            memory_isolation(first + second.replace("TICKS=300", "TICKS=600"), 2)
        with self.assertRaisesRegex(ValueError, "share accelerator time"):
            memory_isolation(first + second.replace("CYCLES=100", "CYCLES=200"), 2)

    def test_hbm_indices_are_reproducible_race_free_permutations(self):
        with tempfile.TemporaryDirectory() as temporary:
            first, second = Path(temporary) / 'first.xml', Path(temporary) / 'second.xml'
            hbm_input(first, 256)
            hbm_input(second, 256)
            self.assertEqual(first.read_bytes(), second.read_bytes())
            encoded = ET.parse(first).getroot().find('testbench').get('indices')
            indices = list(map(int, encoded.strip('{}').split(',')))
            self.assertEqual(sorted(indices), list(range(256)))
            self.assertNotEqual(indices, sorted(indices))

    def test_hbm_requires_more_than_four_concurrent_requesters(self):
        header = ('$scope module bench $end\n$scope module system $end\n'
                  '$var wire 1 C clock $end\n$scope module DUT $end\n'
                  '$var wire 32 B Mout_oe_ram [31:0] $end\n$scope module top $end\n')
        header += ''.join(f'$var wire 1 A{i} sig_in_Mout_oe_ram_{i} $end\n' for i in range(32))
        header += '$upscope $end\n' * 4 + '$enddefinitions $end\n#0\n0C\n'
        header += ''.join(f'0A{i}\n' for i in range(32))
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / 'trace.vcd'
            for active in (4, 32):
                path.write_text(header + '#5\n' + ''.join(f'1A{i}\n' for i in range(active))
                                + 'b' + '1' * 32 + ' B\n1C\n#10\n0C\n')
                if active == 4:
                    with self.assertRaisesRegex(ValueError, 'insufficient parallel'):
                        parallelism(path, 32, 8, 32)
                else:
                    evidence = parallelism(path, 32, 8, 32)
                    self.assertEqual(evidence['peak_parallel_requests'], 32)
                    self.assertEqual(evidence['active_banks'], list(range(32)))

    def test_success_without_real_ramulator_activity_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'bambu_results.xml').write_text(
                '<application><timing><simulation return_value="0"/></timing>'
                '<evaluation CYCLES="100"/></application>')
            (root / 'run.log').write_text('Ramulator2 successfully initialized\n')
            with self.assertRaisesRegex(ValueError, 'actual Ramulator read completions'):
                validate(root, 0, False, 5000)

    def test_wrong_functional_result_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'bambu_results.xml').write_text(
                '<application><timing><simulation return_value="1"/></timing>'
                '<evaluation CYCLES="100"/></application>')
            with self.assertRaisesRegex(ValueError, 'RTL/golden'):
                validate(root, 0, False, 5000)

    def test_two_banks_used_serially_are_not_parallel_traffic(self):
        header = '''$scope module bench $end
$scope module system $end
$var wire 1 C clock $end
$scope module DUT $end
$var wire 4 B Mout_oe_ram [3:0] $end
$scope module top $end
$var wire 1 A0 sig_in_Mout_oe_ram_0 $end
$var wire 1 A1 sig_in_Mout_oe_ram_1 $end
$var wire 1 A2 sig_in_Mout_oe_ram_2 $end
$var wire 1 A3 sig_in_Mout_oe_ram_3 $end
$upscope $end
$upscope $end
$upscope $end
$upscope $end
$enddefinitions $end
#0
0C
0A0
0A1
0A2
0A3
b0000 B
'''
        serial = '#5\n1A0\nb0001 B\n1C\n#10\n0C\n0A0\n#15\n1A1\nb0010 B\n1C\n#20\n0C\n'
        concurrent = '#5\n1A0\n1A1\nb0011 B\n1C\n#10\n0C\n'
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / 'trace.vcd'
            path.write_text(header + serial)
            with self.assertRaisesRegex(ValueError, 'insufficient parallel'):
                parallelism(path)
            path.write_text(header + concurrent)
            evidence = parallelism(path)
            self.assertEqual(evidence['peak_parallel_requests'], 2)
            self.assertEqual(evidence['active_banks'], [0, 1])


if __name__ == '__main__':
    unittest.main()
