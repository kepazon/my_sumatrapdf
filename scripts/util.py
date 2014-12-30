import os
import re
import subprocess
import sys
import hashlib
import string
import time
import types
import zipfile
import bz2
import shutil


def log(s):
    print(s)
    sys.stdout.flush()


def strip_empty_lines(s):
    s = s.replace("\r\n", "\n")
    lines = [l.strip() for l in s.split("\n") if len(l.strip()) > 0]
    return string.join(lines, "\n")


def trim_str(s):
    if len(s) < 75:
        return (s, False)
    # we don't want to trim if adding "..." would make it bigger than original
    if len(s) < 78:
        return (s, False)
    return (s[:75], True)


def test_for_flag(args, arg, has_data=False):
    if arg not in args:
        if not has_data:
            return False
        for argx in args:
            if argx.startswith(arg + "="):
                args.remove(argx)
                return argx[len(arg) + 1:]
        return None

    if not has_data:
        args.remove(arg)
        return True

    idx = args.index(arg)
    if idx == len(args) - 1:
        return None
    data = args[idx + 1]
    args.pop(idx + 1)
    args.pop(idx)
    return data


def file_sha1(fp):
    data = open(fp, "rb").read()
    m = hashlib.sha1()
    m.update(data)
    return m.hexdigest()


def delete_file(path):
    if os.path.exists(path):
        os.remove(path)


def create_dir(d):
    if not os.path.exists(d):
        os.makedirs(d)
    return d


def verify_path_exists(path):
    if not os.path.exists(path):
        print("path '%s' doesn't exist" % path)
        sys.exit(1)
    return path


def verify_started_in_right_directory():
    if os.path.exists(os.path.join("scripts", "build.py")):
        return
    if os.path.exists(os.path.join(os.getcwd(), "scripts", "build.py")):
        return
    print("This script must be run from top of the source tree")
    sys.exit(1)


def subprocess_flags():
    # this magic disables the modal dialog that windows shows if the process crashes
    # TODO: it doesn't seem to work, maybe because it was actually a crash in a process
    # sub-launched from the process I'm launching. I had to manually disable this in
    # registry, as per http://stackoverflow.com/questions/396369/how-do-i-disable-the-debug-close-application-dialog-on-windows-vista:
    # DWORD HKLM or HKCU\Software\Microsoft\Windows\Windows Error Reporting\DontShowUI = "1"
    # DWORD HKLM or HKCU\Software\Microsoft\Windows\Windows Error Reporting\Disabled = "1"
    # see: http://msdn.microsoft.com/en-us/library/bb513638.aspx
    if sys.platform.startswith("win"):
        import ctypes
        SEM_NOGPFAULTERRORBOX = 0x0002  # From MSDN
        ctypes.windll.kernel32.SetErrorMode(SEM_NOGPFAULTERRORBOX)
        return 0x8000000  # win32con.CREATE_NO_WINDOW?
    return 0


# Apparently shell argument to Popen it must be False on unix/mac and True
# on windows
def shell_arg():
    if os.name == "nt":
        return True
    return False


# will throw an exception if a command doesn't exist
# otherwise returns a tuple:
# (stdout, stderr, errcode)
def run_cmd(*args):
    cmd = " ".join(args)
    print("run_cmd: '%s'" % cmd)
    cmdproc = subprocess.Popen(args, shell=shell_arg(), stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, creationflags=subprocess_flags())
    res = cmdproc.communicate()
    return (res[0], res[1], cmdproc.returncode)


# like run_cmd() but throws an exception if command returns non-0 error code
def run_cmd_throw(*args):
    cmd = " ".join(args)
    print("run_cmd_throw: '%s'" % cmd)

    cmdproc = subprocess.Popen(args, shell=shell_arg(), stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, creationflags=subprocess_flags())
    res = cmdproc.communicate()
    errcode = cmdproc.returncode
    if 0 != errcode:
        print("Failed with error code %d" % errcode)
        if len(res[0]) > 0:
            print("Stdout:\n%s" % res[0])
        if len(res[1]) > 0:
            print("Stderr:\n%s" % res[1])
        raise Exception("'%s' failed with error code %d" % (cmd, errcode))
    return (res[0], res[1])


# work-around a problem with running devenv from command-line:
# http://social.msdn.microsoft.com/Forums/en-US/msbuild/thread/9d8b9d4a-c453-4f17-8dc6-838681af90f4
def kill_msbuild():
    (stdout, stderr, err) = run_cmd("taskkill", "/F", "/IM", "msbuild.exe")
    if err not in (0, 128):  # 0 is no error, 128 is 'process not found'
        print("err: %d\n%s%s" % (err, stdout, stderr))
        print("exiting")
        sys.exit(1)


# get a linear version from git by counting number of commits
def get_git_linear_version():
    subprocess.check_call(["git", "pull"])
    out = subprocess.check_output(["git", "log", "--oneline"])
    lines = [l for l in out.split('\n') if len(l.strip()) > 0]
    # we add 1000 to create a version that is larger than the svn version
    # from the time we used svn
    nLines = len(lines) + 1000
    return nLines


# version line is in the format:
# define CURR_VERSION 1.1
def extract_sumatra_version(file_path):
    content = open(file_path).read()
    ver = re.findall(r'CURR_VERSION (\d+(?:\.\d+)*)', content)[0]
    return ver


def file_remove_try_hard(path):
    removeRetryCount = 0
    while removeRetryCount < 3:
        try:
            os.remove(path)
            return
        except:
            # try to sleep to make the time for the file not be used anymore
            time.sleep(1)
            print "exception: n  %s, n  %s, n  %s n  when trying to remove file %s" % (sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2], path)
        removeRetryCount += 1


def zip_file(dst_zip_file, src_path, in_zip_name=None, compress=True, append=False):
    mode = "w"
    if append:
        mode = "a"
    if compress:
        zf = zipfile.ZipFile(dst_zip_file, mode, zipfile.ZIP_DEFLATED)
    else:
        zf = zipfile.ZipFile(dst_zip_file, mode, zipfile.ZIP_STORED)
    if in_zip_name is None:
        in_zip_name = os.path.basename(src_path)
    zf.write(src_path, in_zip_name)
    zf.close()


def bz_file_compress(src, dst):
    with open(src, "rb") as src_fo:
        with bz2.BZ2File(dst, "w", buffering=16 * 1024 * 1024, compresslevel=9) as dst_fo:
            shutil.copyfileobj(src_fo, dst_fo, length=1 * 1024 * 1024)


def formatInt(x):
    if x < 0:
        return '-' + formatInt(-x)
    result = ''
    while x >= 1000:
        x, r = divmod(x, 1000)
        result = ".%03d%s" % (r, result)
    return "%d%s" % (x, result)


def str2bool(s):
    if s.lower() in ("true", "1"):
        return True
    if s.lower() in ("false", "0"):
        return False
    assert(False)


class Serializable(object):

    def __init__(self, fields, fields_no_serialize, read_from_file=None):
        self.fields = fields
        self.fields_no_serialize = fields_no_serialize
        self.vals = {}

        if read_from_file != None:
            self.from_s(open(read_from_file, "r").read())

    def type_of_field(self, name):
        return type(self.fields[name])

    def from_s(self, s):
        # print(s)
        lines = s.split("\n")
        for l in lines:
            (name, val) = l.split(": ", 1)
            tp = self.type_of_field(name)
            if tp == types.IntType:
                self.vals[name] = int(val)
            elif tp == types.LongType:
                self.vals[name] = long(val)
            elif tp == types.BooleanType:
                self.vals[name] = str2bool(val)
            elif tp in (types.StringType, types.UnicodeType):
                self.vals[name] = val
            else:
                print(name)
                assert(False)

    def to_s(self):
        res = []
        for k, v in self.vals.items():
            if k in self.fields_no_serialize:
                continue
            res.append("%s: %s" % (k, str(v)))
        return string.join(res, "\n")

    def write_to_file(self, filename):
        open(filename, "w").write(self.to_s())

    def compat_types(self, tp1, tp2):
        if tp1 == tp2:
            return True
        num_types = (types.IntType, types.LongType)
        if tp1 in num_types and tp2 in num_types:
            return True
        return False

    def __setattr__(self, k, v):
        if k in self.fields:
            if not self.compat_types(type(v), type(self.fields[k])):
                print("k='%s', %s != %s (type(v) != type(self.fields[k]))" % (
                    k, type(v), type(self.fields[k])))
                assert type(v) == type(self.fields[k])
            self.vals[k] = v
        else:
            super(Serializable, self).__setattr__(k, v)

    def __getattr__(self, k):
        if k in self.vals:
            return self.vals[k]
        if k in self.fields:
            return self.fields[k]
        return super(Serializable, self).__getattribute__(k)


import smtplib
from email.MIMEMultipart import MIMEMultipart
from email.MIMEText import MIMEText


def sendmail(sender, senderpwd, to, subject, body):
    # print("sendmail is disabled"); return
    mail = MIMEMultipart()
    mail['From'] = sender
    toHdr = to
    if isinstance(toHdr, list):
        toHdr = ", ".join(toHdr)
    mail['To'] = toHdr
    mail['Subject'] = subject
    mail.attach(MIMEText(body))
    msg = mail.as_string()
    # print(msg)
    mailServer = smtplib.SMTP("smtp.mandrillapp.com", 587)
    mailServer.ehlo()
    mailServer.starttls()
    mailServer.ehlo()
    mailServer.login(sender, senderpwd)
    mailServer.sendmail(sender, to, msg)
    mailServer.close()


# Some operations, like uploading to s3, require knowing s3 credential
# We store all such information that cannot be publicly known in a file
# config.py. This object is just a wrapper to documents the fields
# and given default values if config.py doesn't exist
class Config(object):

    def __init__(self):
        self.aws_access = None
        self.aws_secret = None
        self.cert_pwd = None
        self.trans_ul_secret = None
        self.notifier_email = None
        self.notifier_email_pwd = None

    def GetNotifierEmailAndPwdMustExist(self):
        assert(None != self.notifier_email and None != self.notifier_email_pwd)
        return (self.notifier_email, self.notifier_email_pwd)

    def HasNotifierEmail(self):
        return self.notifier_email != None and self.notifier_email_pwd != None

    def GetCertPwdMustExist(self):
        assert(None != self.cert_pwd)
        return self.cert_pwd

    def GetTransUploadSecret(self):
        assert(None != self.trans_ul_secret)
        return self.trans_ul_secret

    # TODO: could verify aws creds better i.e. check the lengths
    def GetAwsCredsMustExist(self):
        assert(None != self.aws_access)
        assert(None != self.aws_secret)
        return (self.aws_access, self.aws_secret)

    def HasAwsCreds(self):
        if None is self.aws_access:
            return False
        if None is self.aws_secret:
            return False
        return True


g_config = None
def load_config():
    global g_config
    if g_config != None:
        return g_config
    c = Config()
    try:
        import config
        c.aws_access = config.aws_access
        c.aws_secret = config.aws_secret
        c.cert_pwd = config.cert_pwd
        c.notifier_email = config.notifier_email
        c.notifier_email_pwd = config.notifier_email_pwd
        c.trans_ul_secret = config.trans_ul_secret
    except:
        # it's ok if doesn't exist, we just won't have the config data
        print("no config.py!")
    g_config = c
    return g_config


def test_load_config():
    c = load_config()
    vals = (c.aws_access, c.aws_secret, c.cert_pwd, c.trans_ul_secret)
    print("aws_secret: %s\naws_secret: %s\ncert_pwd: %s\ntrans_ul_secret: %s" %
          vals)


def gob_uvarint_encode(i):
    assert i >= 0
    if i <= 0x7f:
        return chr(i)
    res = ""
    while i > 0:
        b = i & 0xff
        res += chr(b)
        i = i >> 8
    l = 256 - len(res)
    res = res[::-1]  # reverse string
    return chr(l) + res


def gob_varint_encode(i):
    if i < 0:
        i = (~i << 1) | 1
    else:
        i = i << 1
    return gob_uvarint_encode(i)


# data generated with UtilTests.cpp (define GEN_PYTHON_TESTS to 1)
def test_gob():
    assert gob_varint_encode(0) == chr(0)
    assert gob_varint_encode(1) == chr(2)
    assert gob_varint_encode(127) == chr(255) + chr(254)
    assert gob_varint_encode(128) == chr(254) + chr(1) + chr(0)
    assert gob_varint_encode(129) == chr(254) + chr(1) + chr(2)
    assert gob_varint_encode(254) == chr(254) + chr(1) + chr(252)
    assert gob_varint_encode(255) == chr(254) + chr(1) + chr(254)
    assert gob_varint_encode(256) == chr(254) + chr(2) + chr(0)
    assert gob_varint_encode(4660) == chr(254) + chr(36) + chr(104)
    assert gob_varint_encode(74565) == chr(253) + chr(2) + chr(70) + chr(138)
    assert gob_varint_encode(1193046) == chr(253) + \
        chr(36) + chr(104) + chr(172)
    assert gob_varint_encode(19088743) == chr(252) + \
        chr(2) + chr(70) + chr(138) + chr(206)
    assert gob_varint_encode(305419896) == chr(252) + \
        chr(36) + chr(104) + chr(172) + chr(240)
    assert gob_varint_encode(2147483647) == chr(252) + \
        chr(255) + chr(255) + chr(255) + chr(254)
    assert gob_varint_encode(-1) == chr(1)
    assert gob_varint_encode(-2) == chr(3)
    assert gob_varint_encode(-255) == chr(254) + chr(1) + chr(253)
    assert gob_varint_encode(-256) == chr(254) + chr(1) + chr(255)
    assert gob_varint_encode(-257) == chr(254) + chr(2) + chr(1)
    assert gob_varint_encode(-4660) == chr(254) + chr(36) + chr(103)
    assert gob_varint_encode(-74565) == chr(253) + chr(2) + chr(70) + chr(137)
    assert gob_varint_encode(-1193046) == chr(253) + \
        chr(36) + chr(104) + chr(171)
    assert gob_varint_encode(-1197415) == chr(253) + \
        chr(36) + chr(138) + chr(205)
    assert gob_varint_encode(-19158648) == chr(252) + \
        chr(2) + chr(72) + chr(172) + chr(239)
    assert gob_uvarint_encode(0) == chr(0)
    assert gob_uvarint_encode(1) == chr(1)
    assert gob_uvarint_encode(127) == chr(127)
    assert gob_uvarint_encode(128) == chr(255) + chr(128)
    assert gob_uvarint_encode(129) == chr(255) + chr(129)
    assert gob_uvarint_encode(254) == chr(255) + chr(254)
    assert gob_uvarint_encode(255) == chr(255) + chr(255)
    assert gob_uvarint_encode(256) == chr(254) + chr(1) + chr(0)
    assert gob_uvarint_encode(4660) == chr(254) + chr(18) + chr(52)
    assert gob_uvarint_encode(74565) == chr(253) + chr(1) + chr(35) + chr(69)
    assert gob_uvarint_encode(1193046) == chr(253) + \
        chr(18) + chr(52) + chr(86)
    assert gob_uvarint_encode(19088743) == chr(252) + \
        chr(1) + chr(35) + chr(69) + chr(103)
    assert gob_uvarint_encode(305419896) == chr(252) + \
        chr(18) + chr(52) + chr(86) + chr(120)
    assert gob_uvarint_encode(2147483647) == chr(252) + \
        chr(127) + chr(255) + chr(255) + chr(255)
    assert gob_uvarint_encode(2147483648) == chr(252) + \
        chr(128) + chr(0) + chr(0) + chr(0)
    assert gob_uvarint_encode(2147483649) == chr(252) + \
        chr(128) + chr(0) + chr(0) + chr(1)
    assert gob_uvarint_encode(4294967294) == chr(252) + \
        chr(255) + chr(255) + chr(255) + chr(254)
    assert gob_uvarint_encode(4294967295) == chr(252) + \
        chr(255) + chr(255) + chr(255) + chr(255)


# for easy generation of the compact form of storing strings in C
class SeqStrings(object):

    def __init__(self):
        self.strings = {}
        self.strings_seq = ""

    def get_all(self):
        return self.strings_seq + chr(0)

    # Note: this only works if strings are ascii, which is the case for us so
    # far
    def get_all_c_escaped(self):
        s = self.get_all()
        s = s.replace(chr(0), "\\0")
        return '"' + s + '"'

    def add(self, s):
        self.get_offset(s)

    def get_offset(self, s):
        if s not in self.strings:
            self.strings[s] = len(self.strings_seq)
            self.strings_seq = self.strings_seq + s + chr(0)
        return self.strings[s]


(FMT_NONE, FMT_LEFT, FMT_RIGHT) = (0, 1, 2)
def get_col_fmt(col_fmt, col):
    if col >= len(col_fmt):
        return FMT_NONE
    return col_fmt[col]


def fmt_str(s, max, fmt):
    add = max - len(s)
    if fmt == FMT_LEFT:
        return " " * add + s
    elif fmt == FMT_RIGHT:
        return s + " " * add
    return s


"""
[
  ["a",  "bc",   "def"],
  ["ab", "fabo", "d"]
]
=>
[
  ["a ", "bc  ", "def"],
  ["ab", "fabo", "d  "]
]
"""
def fmt_rows(rows, col_fmt=[]):
    col_max_len = {}
    for row in rows:
        for col in range(len(row)):
            el_len = len(row[col])
            curr_max = col_max_len.get(col, 0)
            if el_len > curr_max:
                col_max_len[col] = el_len
    res = []
    for row in rows:
        res_row = []
        for col in range(len(row)):
            s = fmt_str(row[col], col_max_len[col], get_col_fmt(col_fmt, col))
            res_row.append(s)
        res.append(res_row)
    return res

if __name__ == "__main__":
    # test_load_config()
    test_gob()


def plural(n, suff):
    if n == 1:
        return "%d %s" % (n, suff)
    return "%d %ss" % (n, suff)


def pretty_print_secs(secs):
    hrs = 0
    mins = 0
    if secs > 60:
        mins = secs / 60
        secs = secs % 60
    if mins > 60:
        hrs = mins / 60
        mins = mins % 60
    if hrs > 0:
        return "%s %s %s" % (plural(hrs, "hr"), plural(mins, "min"), plural(secs, "sec"))
    if mins > 0:
        return "%s %s" % (plural(mins, "min"), plural(secs, "sec"))
    return "%s" % plural(secs, "sec")
