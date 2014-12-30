"""
Builds test executable(s), runs them and checks for failures.

The conventions are:
 - premake4.lua defines all_tests.sln which contains one or more
   test projects
 - we build Release version of all projects
 - they end up as obj-rel/test_*.exe executables
 - we run all those test executables
 - a test executable returns 0 if all tests passed or > 0 if
   one or more tests failed. Additionally, stderr might contain
   an error message pin-pointing the problem. stderr is used by
   buildbot. stdout can be used for interactive use
"""
import os
import util


def run_premake(action="vs2010"):
    try:
        (out, err, errcode) = util.run_cmd("premake4", action)
        if errcode != 0:
            return out + err
    except:
        return "premake4.exe not in %PATH%"
    return None


def is_test_exe(file_name):
    return file_name.startswith("test_") and file_name.endswith(".exe")


def is_empty_str(s):
    return s == None or len(s) == 0


def fmt_out_err(out, err):
    if is_empty_str(out) and is_empty_str(err):
        return ""
    if is_empty_str(out):
        return err
    if is_empty_str(err):
        return out
    return out + "\n" + err

# returns None if all tests succeeded or an error string if one
# or more tests failed
# assumes current directory is top-level sumatra dir


def run_tests2():
    if not os.path.exists("premake4.lua"):
        return "premake4.lua doesn't exist in current directory (%s)" % os.getcwd()
    err = run_premake()
    if err != None:
        return err
    p = os.path.join("vs-premake", "all_tests.sln")
    if not os.path.exists(p):
        return "%s doesn't exist" % p
    os.chdir("vs-premake")
    try:
        util.kill_msbuild()
    except:
        return "util.kill_msbuild() failed"
    try:
        (out, err, errcode) = util.run_cmd("devenv",
                                           "all_tests.sln", "/build", "Release")
        if errcode != 0:
            return "devenv.exe failed to build all_tests.sln\n" + fmt_out_err(out, err)
    except:
        return "devenv.exe not found"
    p = os.path.join("..", "obj-rel")
    os.chdir(p)
    test_files = [f for f in os.listdir(".") if is_test_exe(f)]
    print("Running %d test executables" % len(test_files))
    for f in test_files:
        try:
            (out, err, errcode) = util.run_cmd(f)
            if errcode != 0:
                return "%s failed with:\n%s" % (f, fmt_out_err(out, err))
            print(fmt_out_err(out, err))
        except:
            return "%s failed to run" % f
    return None


def run_tests():
    d = os.getcwd()
    res = run_tests2()
    os.chdir(d)
    return res


def main():
    err = run_tests()
    if None == err:
        print("Tests passed!")
    else:
        print("Tests failed. Error message:\n" + err)

if __name__ == "__main__":
    main()
