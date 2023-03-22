#!/usr/bin/env python3
import subprocess
# 17
lv0_1 = {
    "return": (3, False),
    "decl_int": (2, False),
    "decl_float": (2, False),
    "decl_int_array": (2, False),
    "decl_float_array": (2, False),
    "input": (2, True),
    "output_float": (2, False),
    "output_int": (2, False),
}

# 18
lv0_2 = {
    "num_add_int": (0.5, False),
    "num_sub_int": (0.5, False),
    "num_mul_int": (0.5, False),
    "num_div_int": (0.5, False),
    "num_add_float": (0.5, False),
    "num_sub_float": (0.5, False),
    "num_mul_float": (0.5, False),
    "num_div_float": (0.5, False),
    "num_add_mixed": (0.5, False),
    "num_sub_mixed": (0.5, False),
    "num_mul_mixed": (0.5, False),
    "num_div_mixed": (0.5, False),
    "num_comp1": (1.5, False),
    "num_le_int": (0.5, False),
    "num_lt_int": (0.5, False),
    "num_ge_int": (0.5, False),
    "num_gt_int": (0.5, False),
    "num_eq_int": (0.5, False),
    "num_neq_int": (0.5, False),
    "num_le_float": (0.5, False),
    "num_lt_float": (0.5, False),
    "num_ge_float": (0.5, False),
    "num_gt_float": (0.5, False),
    "num_eq_float": (0.5, False),
    "num_neq_float": (0.5, False),
    "num_le_mixed": (0.5, False),
    "num_lt_mixed": (0.5, False),
    "num_ge_mixed": (0.5, False),
    "num_gt_mixed": (0.5, False),
    "num_eq_mixed": (0.5, False),
    "num_neq_mixed": (0.5, False),
    "num_comp2": (1.5, False),
}

# 31
lv1 = {
    "assign_int_var_local": (1, False),
    "assign_int_array_local": (2, False),
    "assign_int_var_global": (1, False),
    "assign_int_array_global": (2, False),
    "assign_float_var_local": (1, False),
    "assign_float_array_local": (2, False),
    "assign_float_var_global": (1, False),
    "assign_float_array_global": (2, False),
    "assign_cmp": (1, False),
    "innout": (1, True),
    "idx_float": (1, False),
    "negidx_int": (1, False),
    "negidx_float": (1, False),
    "negidx_intfuncall": (1, False),
    "negidx_floatfuncall": (1, False),
    "negidx_voidfuncall": (1, False),
    "selection1": (1.5, False),
    "selection2": (1.5, False),
    "selection3": (1.5, False),
    "iteration1": (1.5, False),
    "iteration2": (1.5, False),
    "scope": (1.5, False),
    "transfer_float_to_int": (1, False),
    "transfer_int_to_float": (1, False),
}

# 23
lv2 = {
    "funcall_chain": (2, False),
    "assign_chain": (2, False),
    "funcall_var": (2, False),
    "funcall_int_array": (2, False),
    "funcall_float_array": (2, False),
    "funcall_array_array": (2, False),
    "return_in_middle1": (2, False),
    "return_in_middle2": (2, False),
    "funcall_type_mismatch1": (2, False),
    "funcall_type_mismatch2": (2, False),
    "return_type_mismatch1": (1.5, False),
    "return_type_mismatch2": (1.5, False),
}

# 11
lv3 = {
    "complex1": (3, False),
    "complex2": (3, True),
    "complex3": (2, True),
    "complex4": (3, False),
}

suite = [
    ("lv0_1", lv0_1, 0),
    ("lv0_2", lv0_2, 0),
    ("lv1", lv1, 0),
    ("lv2", lv2, 0),
    ("lv3", lv3, 0)
]


def eval():
    f = open("eval_result", 'w')
    EXE_PATH = "../../build/cminusfc"
    TEST_BASE_PATH = "./testcases/"
    ANSWER_BASE_PATH = "./answers/"
    total_points = 0
    for level in suite:
        lv_points = 0
        has_bonus = True
        level_name = level[0]
        bonus = level[2]
        cases = level[1]
        f.write('===========%s START========\n' % level_name)
        for case in cases:
            f.write('%s:' % case)
            TEST_PATH = TEST_BASE_PATH + level_name + "/" + case
            ANSWER_PATH = ANSWER_BASE_PATH + level_name + "/" + case
            score = cases[case][0]
            need_input = cases[case][1]

            COMMAND = [TEST_PATH]

            try:
                result = subprocess.run([EXE_PATH, TEST_PATH + ".cminus"], stderr=subprocess.PIPE, timeout=1)
            except Exception as _:
                f.write('\tFail\n')
                continue

            if result.returncode == 0:
                input_option = None
                if need_input:
                    with open(ANSWER_PATH + ".in", "rb") as fin:
                        input_option = fin.read()

                try:
                    result = subprocess.run(COMMAND, input=input_option, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=1)
                    with open(ANSWER_PATH + ".out", "rb") as fout:
                        if result.stdout == fout.read():
                            f.write('\tSuccess\n')
                            lv_points += score
                        else:
                            f.write('\tFail\n')
                            has_bonus = False
                except Exception as _:
                    f.write('\tFail\n')
                    has_bonus = False
                finally:
                    subprocess.call(["rm", "-rf", TEST_PATH, TEST_PATH + ".o"])

            else:
                f.write('\tFail\n')
                has_bonus = False

        if has_bonus:
            lv_points += bonus

        total_points += lv_points
        f.write('points of %s is: %d\n' % (level_name, lv_points))
        f.write('===========%s END========\n\n' % level_name)
    f.write('total points: %d\n' % total_points)


if __name__ == "__main__":
    eval()
