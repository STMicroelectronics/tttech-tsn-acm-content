#!/usr/bin/python
#
# Run these tests on target device.
# Do not forget to provide python-unittest and python-mmap,
# e.g. add
#   IMAGE_INSTALL_append = " python-unittest python-mmap"
# to your local.conf of your Yocto build!
#
import unittest
import os
import struct
import io
import re
import subprocess
import sys
import mmap
import random
import stat

basedir = "/sys/devices/ngn/"
oldDriver = False
newDriver = False
compatible = ''
debugIP = False

# initialize format table for struct pack/unpack access
fmt = [None, '<B', '<H', None, '<I', None, None, None, '<Q']

# decorator to suppress test functions conditionally
#def suppress_if(cond, msg=None):
#    def noop_decorator(func):
#        return func  # pass through
#
#    def neutered_function(func):
#        def neutered(*args, **kwargs):
#            return SkipTest("skipped")
#        return neutered
#    return neutered_function if cond else noop_decorator

#
# see https://stackoverflow.com/questions/1323455/python-unit-test-with-base-and-sub-class
#
class ACMBaseTests:
    @staticmethod
    def detectDriver(name):
        p1 = subprocess.Popen(["lsmod"], stdout=subprocess.PIPE)
        p2 = subprocess.Popen(["grep", name], stdin=p1.stdout,
                              stdout=subprocess.PIPE)
        p1.stdout.close()  # Allow p1 to receive a SIGPIPE if p2 exits.
        output = p2.communicate()[0]
        
        pattern = re.compile("^" + name)
        return pattern.match(output)

    @staticmethod
    def resetFpga():
        f = io.FileIO(os.path.join(basedir, "config_bin/clear_all_fpga"), 'w')
        f.write(struct.pack('<I', 0x13F72288));
        f.close()

    @staticmethod
    def is_debugIP():
        return debugIP

    class ACMTest(unittest.TestCase):
        """Basic ACM test class just providing reset functionality
        """
        items = None
        itemsize = None
        defaultValue = None
        defaultValueMask = ~0;
        filename = None
        readonly = False

        nr_bypass = 2

        def setUp(self):
            super(ACMBaseTests.ACMTest, self).setUp()
            #print "Running", self._testMethodName
            self.longMessage = True
            ACMBaseTests.resetFpga()
            if self.filename:
                self.rfile = io.FileIO(os.path.join(basedir, self.filename), 'r')
                if not self.readonly:
                    self.wfile = io.FileIO(os.path.join(basedir, self.filename), 'w')
            Klog.clear()

        def tearDown(self):
            if self.rfile:
                self.rfile.close()
                if not self.readonly:
                    self.wfile.close()
            ACMBaseTests.resetFpga()
            super(ACMBaseTests.ACMTest, self).tearDown()

        # common tests
        def test_wrong_size_alignment(self):
            if self.items < 2 or self.itemsize < 2:
                self.skipTest("cannot test size alignment violation on trivial items")
            self.rfile.seek(0)
            with self.assertRaises(IOError):
                self.rfile.read(self.itemsize - 1)
            # check for kernel log
            self.assertRegexpMatches (Klog.readclear(), ".*Required length \([0-9]*\) not aligned to [0-9]*")


        def test_wrong_offset_alignment(self):
            # care for data types with padded data field
            try:
                s = self.itemsize_sysfs
            except AttributeError:
                s = self.itemsize
            if self.items < 2 or s < 2:
                self.skipTest("cannot test offset alignment violation on trivial items")
            self.rfile.seek(s - 1)
            with self.assertRaises(IOError):
                self.rfile.read(s)
            # check for kernel log
            self.assertRegexpMatches (Klog.readclear(), ".*Trying to access [0-9]* bytes with offset [0-9]*: only [0-9]* bytes available|.*Offset \([0-9]*\) not aligned to [0-9]*")

        def test_read_all_items_sequentially(self):
            # care for data types with padded data field
            try:
                s = self.itemsize_sysfs
            except AttributeError:
                s = self.itemsize

            for i in range(self.items):
                self.rfile.seek(i * s)
                data = self.rfile.read(s);
                if self.itemsize in (1, 2, 4, 8):
                    val = struct.unpack(fmt[self.itemsize], data)[0]
                    self.assertEqual(val & self.defaultValueMask, self.defaultValue, 'at element number {0}'.format(i))
                else:
                    # bytewise compare
                    for j in range(self.itemsize):
                        val = struct.unpack('B', data[j])[0]
                        self.assertEqual(val & self.defaultValueMask, self.defaultValue, 'at element number {0}'.format(i))
            # make sure there is no kernel log
            self.assertEqual(Klog.readclear(), "")

        #
        def test_read_all_items_en_bloc(self):
            self.rfile.seek(0)
            # we probably need multiple reads on big data items
            data = ""
            while len(data) < self.itemsize * self.items:
                data = data + self.rfile.read(self.itemsize * self.items)
            for i in range(self.items):
                itemdata = data[i * self.itemsize : (i + 1) * self.itemsize]
                if self.itemsize in (1, 2, 4, 8):
                    val = struct.unpack(fmt[self.itemsize], itemdata)[0]
                    self.assertEqual(val & self.defaultValueMask, self.defaultValue, 'at element number {0}'.format(i))
                else:
                    # bytewise compare
                    for j in range(self.itemsize):
                        val = struct.unpack('B', data[j])[0]
                        self.assertEqual(val & self.defaultValueMask, self.defaultValue, 'at element number {0}'.format(i))
            # make sure there is no kernel log
            self.assertEqual(Klog.readclear(), "")

    class ACMTestDual(unittest.TestCase):
        """Basic ACM test class just providing reset functionality
        """
        defaultValue = None
        filename = None
        compatibleList = []
        incompatibleList = []

        def setUp(self):
            super(ACMBaseTests.ACMTestDual, self).setUp()
            #print "Running", self._testMethodName
            ACMBaseTests.resetFpga()
            Klog.clear()

        def test_access_module0(self):
            if (len(self.compatibleList) > 0 and compatible not in self.compatibleList) or compatible in self.incompatibleList:
                self.skipTest('Not compatible with %s' % compatible)
            f = io.FileIO(os.path.join(basedir, self.filename + "_M0"), 'r')
            val = int(f.read().rstrip("\n\r"),0)
            self.assertEqual(int(self.defaultValue, 0), val)
            f.close()
            # make sure there is no kernel log
            self.assertEqual(Klog.readclear(), "")

        def test_access_module1(self):
            if (len(self.compatibleList) > 0 and compatible not in self.compatibleList) or compatible in self.incompatibleList:
                self.skipTest('Not compatible with %s' % compatible)
            f = io.FileIO(os.path.join(basedir, self.filename + "_M1"), 'r')
            val = int(f.read().rstrip("\n\r"),0)
            self.assertEqual(int(self.defaultValue, 0), val)
            f.close()
            # make sure there is no kernel log
            self.assertEqual(Klog.readclear(), "")

    class ACMTestDualDebug(ACMTestDual):
        """Extended Basic ACM test class for debug IP only
        """
        defaultValue = None
        filename = None
        compatibleList = []
        incompatibleList = ['ttt,acm-1.0', 'ttt,acm-2.0']

        def setUp(self):
            if ACMBaseTests.is_debugIP():
                super(ACMBaseTests.ACMTestDualDebug, self).setUp()

        def test_access_module0(self):
            if ACMBaseTests.is_debugIP():
                super(ACMBaseTests.ACMTestDualDebug, self).test_access_module0()
            else:
                self.skipTest('for debug IP only')

        def test_access_module1(self):
            if ACMBaseTests.is_debugIP():
                super(ACMBaseTests.ACMTestDualDebug, self).test_access_module1()
            else:
                self.skipTest('for debug IP only')

    class ACMTestExistsReadable(unittest.TestCase):
        filename = None
        compatibleList = []
        incompatibleList = []

        def test_readaccess(self):
            if (len(self.compatibleList) > 0 and compatible not in self.compatibleList) or compatible in self.incompatibleList:
                self.skipTest('Not compatible with %s' % compatible)
            try:
                f = io.FileIO(os.path.join(basedir, self.filename), 'r')
                val = f.read()
            except:
                self.fail('No read access')
            f.close()

class ACMTestDeviceId(ACMBaseTests.ACMTestExistsReadable):
    filename = 'status/device_id'
    incompatibleList = "ttt,acm-1.0"

class ACMTestVersionId(ACMBaseTests.ACMTestExistsReadable):
    filename = 'status/version_id'
    incompatibleList = "ttt,acm-1.0"

class ACMTestRevisionId(ACMBaseTests.ACMTestExistsReadable):
    filename = 'status/revision_id'
    incompatibleList = "ttt,acm-1.0"

class ACMTestExtendedStatus(ACMBaseTests.ACMTestExistsReadable):
    filename = 'status/extended_status'
    incompatibleList = "ttt,acm-1.0"

class ACMTestTestmoduleEnable(ACMBaseTests.ACMTestExistsReadable):
    filename = 'status/testmodule_enable'
    incompatibleList = "ttt,acm-1.0"

class ACMTestTimeFreq(ACMBaseTests.ACMTestExistsReadable):
    filename = 'status/time_freq'
    incompatibleList = "ttt,acm-1.0"

class ACMTestMsgBufMemsize(ACMBaseTests.ACMTestExistsReadable):
    filename = 'status/msgbuf_memsize'
    incompatibleList = "ttt,acm-1.0"

class ACMTestCtrlGatherDelay(ACMBaseTests.ACMTest):
    items = 2
    itemsize = 4
    defaultValue = 0
    filename = "config_bin/cntl_gather_delay"

    def test_wrong_size_alignment(self):
        super(ACMTestCtrlGatherDelay, self).test_wrong_size_alignment()


class ACMTestCtrlConnectionMode(ACMBaseTests.ACMTest):
    items = 2
    itemsize = 4
    defaultValue = 1
    filename = "config_bin/cntl_connection_mode"

    def test_wrong_size_alignment(self):
        super(ACMTestCtrlConnectionMode, self).test_wrong_size_alignment()


class ACMTestCtrlIngressPolicingEnable(ACMBaseTests.ACMTest):
    items = 2
    itemsize = 4
    defaultValue = 0
    filename = "config_bin/cntl_ingress_policing_enable"

    def test_wrong_size_alignment(self):
        super(ACMTestCtrlIngressPolicingEnable, self).test_wrong_size_alignment()


class ACMTestCtrlLayer7Enable(ACMBaseTests.ACMTest):
    items = 2
    itemsize = 4
    defaultValue = 0
    filename = "config_bin/cntl_layer7_enable"

    def test_wrong_size_alignment(self):
        super(ACMTestCtrlLayer7Enable, self).test_wrong_size_alignment()


class ACMTestCtrlLayer7Length(ACMBaseTests.ACMTest):
    items = 2
    itemsize = 4
    defaultValue = 0
    filename = "config_bin/cntl_layer7_length"

    def test_wrong_size_alignment(self):
        super(ACMTestCtrlLayer7Length, self).test_wrong_size_alignment()


class ACMTestCtrlLookupEnable(ACMBaseTests.ACMTest):
    items = 2
    itemsize = 4
    defaultValue = 0
    filename = "config_bin/cntl_lookup_enable"

    def test_wrong_size_alignment(self):
        super(ACMTestCtrlLookupEnable, self).test_wrong_size_alignment()


class ACMTestCtrlNGNEnable(ACMBaseTests.ACMTest):
    items = 2
    itemsize = 4
    defaultValue = 0
    filename = "config_bin/cntl_ngn_enable"

    def test_wrong_size_alignment(self):
        super(ACMTestCtrlNGNEnable, self).test_wrong_size_alignment()


class ACMTestCtrlSpeed(ACMBaseTests.ACMTest):
    items = 2
    itemsize = 4
    defaultValue = 1
    filename = "config_bin/cntl_speed"

    def test_wrong_size_alignment(self):
        super(ACMTestCtrlSpeed, self).test_wrong_size_alignment()


class ACMTestConfigState(ACMBaseTests.ACMTest):
    items = 1
    itemsize = 4
    defaultValue = 1
    filename = "config_bin/config_state"


class ACMTestConstBuffer(ACMBaseTests.ACMTest):
    items = 2
    itemsize = 0x1000
    defaultValue = 0
    filename = "config_bin/const_buffer"

    def test_wrong_size_alignment(self):
        super(ACMTestConstBuffer, self).test_wrong_size_alignment()


class ACMTestEmergencyDisable(ACMBaseTests.ACMTest):
    items = 2
    itemsize = 2
    defaultValue = 0x3
    filename = "config_bin/emergency_disable"


class ACMTestGatherDMA(ACMBaseTests.ACMTest):
    items_per_bypass = 0x100
    items = ACMBaseTests.ACMTest.nr_bypass * items_per_bypass
    itemsize = 4
    itemsize_sysfs = itemsize
    itemoffs = itemsize
    defaultValue = 0
    filename = "config_bin/gather_dma"

    def test_wrong_size_alignment(self):
        super(ACMTestGatherDMA, self).test_wrong_size_alignment()

    def test_read_entry_with_arbitrary_values(self):
        itemnr = random.randint(0, self.items - 1)
        itemdata = bytearray((random.getrandbits(8) for i in xrange(self.itemsize_sysfs)))
        
        for offs in range(0, self.itemsize, 4):
            AddressMap["Bypass0"].write(33 * self.itemoffs + 0x5000 + offs,
                                        struct.unpack("<I", itemdata[offs:offs+4]))
        self.rfile.seek(33 * self.itemsize_sysfs);
        rdata = self.rfile.read(self.itemsize_sysfs)
        # ignore padding data
        self.assertEqual(itemdata[0:self.itemsize], rdata)

        for offs in range(0, self.itemsize, 4):
            AddressMap["Bypass1"].write(10 * self.itemoffs + 0x5000 + offs,
                                        struct.unpack("<I", itemdata[offs:offs+4]))
        self.rfile.seek((10 + self.items_per_bypass) * self.itemsize_sysfs);
        rdata = self.rfile.read(self.itemsize_sysfs)
        # ignore padding data
        self.assertEqual(itemdata[0:self.itemsize], rdata)
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")

    def test_write_entry_with_arbitrary_values(self):
        itemnr = random.randint(0, self.items - 1)
        itemdata = bytearray((random.getrandbits(8) for i in xrange(self.itemsize_sysfs)))

        self.wfile.seek(45 * self.itemsize_sysfs);
        self.wfile.write(itemdata)
        rdata = AddressMap["Bypass0"].read(45 * self.itemoffs + 0x5000, self.itemsize / 4)
        for offs in range(0, self.itemsize, 4):
            self.assertEqual(itemdata[offs:offs+4], struct.pack("<I",rdata[offs / 4 ]))
 
        self.wfile.seek((self.items_per_bypass + 20) * self.itemsize_sysfs);
        self.wfile.write(itemdata)
        rdata = AddressMap["Bypass1"].read(20 * self.itemoffs + 0x5000, self.itemsize / 4)
        for offs in range(0, self.itemsize, 4):
            self.assertEqual(itemdata[offs:offs+4], struct.pack("<I",rdata[offs / 4 ]))
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")


class ACMTestLayer7Mask(ACMBaseTests.ACMTest):
    items_per_bypass = 0x10
    items = ACMBaseTests.ACMTest.nr_bypass * items_per_bypass
    itemsize = 0x70
    itemsize_sysfs = 0x80
    itemoffs = 0x100
    defaultValue = 0
    filename = "config_bin/layer7_mask"

    def test_read_entry_with_arbitrary_values(self):
        itemnr = random.randint(0, self.items - 1)
        itemdata = bytearray((random.getrandbits(8) for i in xrange(self.itemsize_sysfs)))
        
        for offs in range(0, self.itemsize, 4):
            AddressMap["Bypass0"].write(5 * self.itemoffs + 0 + 0x90 + offs,
                                        struct.unpack("<I", itemdata[offs:offs+4]))
        self.rfile.seek(5 * self.itemsize_sysfs);
        rdata = self.rfile.read(self.itemsize_sysfs)
        # ignore padding data
        self.assertEqual(itemdata[0:self.itemsize], rdata[0:self.itemsize])

        for offs in range(0, self.itemsize, 4):
            AddressMap["Bypass1"].write(3 * self.itemoffs + 0 + 0x90 + offs,
                                        struct.unpack("<I", itemdata[offs:offs+4]))
        self.rfile.seek((3 + self.items_per_bypass) * self.itemsize_sysfs);
        rdata = self.rfile.read(self.itemsize_sysfs)
        # ignore padding data
        self.assertEqual(itemdata[0:self.itemsize], rdata[0:self.itemsize])
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")

    def test_write_entry_with_arbitrary_values(self):
        itemnr = random.randint(0, self.items - 1)
        itemdata = bytearray((random.getrandbits(8) for i in xrange(self.itemsize_sysfs)))
 
        self.wfile.seek(8 * self.itemsize_sysfs);
        self.wfile.write(itemdata)
        rdata = AddressMap["Bypass0"].read(8 * self.itemoffs + 0 + 0x90, self.itemsize / 4)
        for offs in range(0, self.itemsize, 4):
            self.assertEqual(itemdata[offs:offs+4], struct.pack("<I",rdata[offs / 4 ]))
 
        self.wfile.seek((self.items_per_bypass + 2) * self.itemsize_sysfs);
        self.wfile.write(itemdata)
        rdata = AddressMap["Bypass1"].read(2 * self.itemoffs + 0 + 0x90, self.itemsize / 4)
        for offs in range(0, self.itemsize, 4):
            self.assertEqual(itemdata[offs:offs+4], struct.pack("<I",rdata[offs / 4 ]))
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")


class ACMTestLayer7Pattern(ACMBaseTests.ACMTest):
    items_per_bypass = 0x10
    items = ACMBaseTests.ACMTest.nr_bypass * items_per_bypass
    itemsize = 0x70
    itemsize_sysfs = 0x80
    itemoffs = 0x100
    defaultValue = 0
    filename = "config_bin/layer7_pattern"

    def test_wrong_size_alignment(self):
        super(ACMTestLayer7Pattern, self).test_wrong_size_alignment()

    def test_read_entry_with_arbitrary_values(self):
        itemnr = random.randint(0, self.items - 1)
        itemdata = bytearray((random.getrandbits(8) for i in xrange(self.itemsize_sysfs)))
        
        for offs in range(0, self.itemsize, 4):
            AddressMap["Bypass0"].write(5 * self.itemoffs + 0 + 0x10 + offs,
                                        struct.unpack("<I", itemdata[offs:offs+4]))
        self.rfile.seek(5 * self.itemsize_sysfs);
        rdata = self.rfile.read(self.itemsize_sysfs)
        # ignore padding data
        self.assertEqual(itemdata[0:self.itemsize], rdata[0:self.itemsize])

        for offs in range(0, self.itemsize, 4):
            AddressMap["Bypass1"].write(3 * self.itemoffs + 0 + 0x10 + offs,
                                        struct.unpack("<I", itemdata[offs:offs+4]))
        self.rfile.seek((3 + self.items_per_bypass) * self.itemsize_sysfs);
        rdata = self.rfile.read(self.itemsize_sysfs)
        # ignore padding data
        self.assertEqual(itemdata[0:self.itemsize], rdata[0:self.itemsize])
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")

    def test_write_entry_with_arbitrary_values(self):
        itemnr = random.randint(0, self.items - 1)
        itemdata = bytearray((random.getrandbits(8) for i in xrange(self.itemsize_sysfs)))
 
        self.wfile.seek(8 * self.itemsize_sysfs);
        self.wfile.write(itemdata)
        rdata = AddressMap["Bypass0"].read(8 * self.itemoffs + 0 + 0x10, self.itemsize / 4)
        for offs in range(0, self.itemsize, 4):
            self.assertEqual(itemdata[offs:offs+4], struct.pack("<I",rdata[offs / 4 ]))
 
        self.wfile.seek((self.items_per_bypass + 2) * self.itemsize_sysfs);
        self.wfile.write(itemdata)
        rdata = AddressMap["Bypass1"].read(2 * self.itemoffs + 0 + 0x10, self.itemsize / 4)
        for offs in range(0, self.itemsize, 4):
            self.assertEqual(itemdata[offs:offs+4], struct.pack("<I",rdata[offs / 4 ]))
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")

class ACMTestLookupMask(ACMBaseTests.ACMTest):
    items_per_bypass = 0x10
    items = ACMBaseTests.ACMTest.nr_bypass * items_per_bypass
    itemsize = 0x10
    itemoffs = 0x100
    defaultValue = 0
    filename = "config_bin/lookup_mask"

    def test_wrong_size_alignment(self):
        super(ACMTestLookupMask, self).test_wrong_size_alignment()

    def test_read_entry_with_arbitrary_values(self):
        itemnr = random.randint(0, self.items - 1)
        itemdata = bytearray((random.getrandbits(8) for i in xrange(self.itemsize)))

        for offs in range(0, self.itemsize, 4):
            AddressMap["Bypass0"].write(5 * self.itemoffs + 0 + 0x80 + offs,
                                        struct.unpack("<I", itemdata[offs:offs+4]))
        self.rfile.seek(5 * self.itemsize);
        rdata = self.rfile.read(self.itemsize)
        self.assertEqual(itemdata, rdata)

        for offs in range(0, self.itemsize, 4):
            AddressMap["Bypass1"].write(7 * self.itemoffs + 0 + 0x80 + offs,
                                        struct.unpack("<I", itemdata[offs:offs+4]))
        self.rfile.seek((7 + self.items_per_bypass) * self.itemsize);
        rdata = self.rfile.read(self.itemsize)
        self.assertEqual(itemdata, rdata)
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")

    def test_write_entry_with_arbitrary_values(self):
        itemnr = random.randint(0, self.items - 1)
        itemdata = bytearray((random.getrandbits(8) for i in xrange(self.itemsize)))

        self.wfile.seek(8 * self.itemsize);
        self.wfile.write(itemdata)
        rdata = AddressMap["Bypass0"].read(8 * self.itemoffs + 0 + 0x80, self.itemsize / 4)
        for offs in range(0, self.itemsize, 4):
            self.assertEqual(itemdata[offs:offs+4], struct.pack("<I",rdata[offs / 4 ]))

        self.wfile.seek((self.items_per_bypass + 2) * self.itemsize);
        self.wfile.write(itemdata)
        rdata = AddressMap["Bypass1"].read(2 * self.itemoffs + 0 + 0x80, self.itemsize / 4)
        for offs in range(0, self.itemsize, 4):
            self.assertEqual(itemdata[offs:offs+4], struct.pack("<I",rdata[offs / 4 ]))
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")


class ACMTestLookupPattern(ACMBaseTests.ACMTest):
    items_per_bypass = 0x10
    items = ACMBaseTests.ACMTest.nr_bypass * items_per_bypass
    itemsize = 0x10
    itemoffs = 0x100
    defaultValue = 0
    filename = "config_bin/lookup_pattern"


    def test_wrong_size_alignment(self):
        super(ACMTestLookupPattern, self).test_wrong_size_alignment()

    def test_read_entry_with_arbitrary_values(self):
        itemnr = random.randint(0, self.items - 1)
        itemdata = bytearray((random.getrandbits(8) for i in xrange(self.itemsize)))

        for offs in range(0, self.itemsize, 4):
            AddressMap["Bypass0"].write(5 * self.itemoffs + 0 + offs,
                                        struct.unpack("<I", itemdata[offs:offs+4]))
        self.rfile.seek(5 * self.itemsize);
        rdata = self.rfile.read(self.itemsize)
        self.assertEqual(itemdata, rdata)

        for offs in range(0, self.itemsize, 4):
            AddressMap["Bypass1"].write(7 * self.itemoffs + 0 + offs,
                                        struct.unpack("<I", itemdata[offs:offs+4]))
        self.rfile.seek((7 + self.items_per_bypass) * self.itemsize);
        rdata = self.rfile.read(self.itemsize)
        self.assertEqual(itemdata, rdata)
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")

    def test_write_entry_with_arbitrary_values(self):
        itemnr = random.randint(0, self.items - 1)
        itemdata = bytearray((random.getrandbits(8) for i in xrange(self.itemsize)))

        self.wfile.seek(8 * self.itemsize);
        self.wfile.write(itemdata)
        rdata = AddressMap["Bypass0"].read(8 * self.itemoffs + 0, self.itemsize / 4)
        for offs in range(0, self.itemsize, 4):
            self.assertEqual(itemdata[offs:offs+4], struct.pack("<I",rdata[offs / 4 ]))

        self.wfile.seek((self.items_per_bypass + 2) * self.itemsize);
        self.wfile.write(itemdata)
        rdata = AddressMap["Bypass1"].read(2 * self.itemoffs + 0, self.itemsize / 4)
        for offs in range(0, self.itemsize, 4):
            self.assertEqual(itemdata[offs:offs+4], struct.pack("<I",rdata[offs / 4 ]))
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")


class ACMTestMsgBufAlias(ACMBaseTests.ACMTest):
    items = 32
    itemsize = 0x40
    defaultValue = 0
    filename = "config_bin/msg_buff_alias"

    def test_read_all_items_sequentially(self):
        for i in range(self.items):
            self.rfile.seek(i * self.itemsize)
            self.rfile.read(self.itemsize)
            # always 0 bytes read from an empty message buffer descriptor
            self.assertEqual(self.rfile.tell(), i * self.itemsize)
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")

    def test_read_all_items_en_bloc(self):
        self.rfile.seek(0)
        self.rfile.read(self.itemsize * self.items)
        # always 0 bytes read from an empty message buffer descriptor
        self.assertEqual(self.rfile.tell(), 0)
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")


    def test_write_with_read_verification(self):
        if newDriver:
            self.skipTest("new driver cannot set alias without activated buffer")

        idx = random.randint(0, self.items - 1)
        id = long(4711)
        bufname="testbuffer"
        data = struct.pack("<BQ55s", idx, id, bufname)
        self.wfile.seek(idx * self.itemsize);
        self.wfile.write(data)
        # read back
        self.rfile.seek(idx * self.itemsize)
        rdata = self.rfile.read(self.itemsize)
        ridx, rid, rname = struct.unpack("<BQ55s", rdata)
        # remove trailing null bytes
        rname = rname.rstrip('\x00') 
        self.assertEqual(idx, ridx)
        self.assertEqual(id, rid)
        self.assertEqual(bufname, rname)
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")

    def test_activate_device(self):
        # first configure activate message buffer 7
        msgbuf = 7
        msgbufsize= 1536
        msgbufoffs = 2048
        msgbufdesc = (1 << 31) | (((msgbufsize / 4) - 1) << 21) | msgbufoffs
        msgbuffile = io.FileIO(os.path.join(basedir, ACMTestMsgBufDesc.filename), 'w')
        msgbuffile.seek(msgbuf * ACMTestMsgBufDesc.itemsize);
        msgbuffile.write(struct.pack(fmt[ACMTestMsgBufDesc.itemsize], msgbufdesc))
        msgbuffile.close()
        # alias configuration
        id = 42
        msgbufalias = "messagebuffer07"
        # verify there is no device yet
        devname = os.path.join("/dev/", msgbufalias)
        self.assertRaises(OSError, os.stat, devname)
        # now configure alias and thus activate device
        self.wfile.seek(msgbuf * self.itemsize);
        self.wfile.write(struct.pack("<BQ55s", msgbuf, id, msgbufalias))
        # verify there is a device now
        self.assertTrue(stat.S_ISCHR(os.stat(devname).st_mode))
        # read back alias for verification
        self.rfile.seek(msgbuf * self.itemsize)
        rmsgbuf, rid, rmsgbufalias = struct.unpack("<BQ55s", self.rfile.read(self.itemsize))
        self.assertEqual(msgbuf, rmsgbuf)
        self.assertEqual(id, rid)
        self.assertEqual(msgbufalias, rmsgbufalias.rstrip('\x00'))
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")

    def test_open_device(self):
        # first configure activate message buffer 7
        msgbuf = 7
        msgbufsize= 1536
        msgbufoffs = 2048
        msgbufdesc = (1 << 31) | (((msgbufsize / 4) - 1) << 21) | msgbufoffs
        msgbuffile = io.FileIO(os.path.join(basedir, ACMTestMsgBufDesc.filename), 'w')
        msgbuffile.seek(msgbuf * ACMTestMsgBufDesc.itemsize);
        msgbuffile.write(struct.pack(fmt[ACMTestMsgBufDesc.itemsize], msgbufdesc))
        msgbuffile.close()
        # alias configuration
        id = 42
        msgbufalias = "messagebuffer07"
        # verify there is no device yet
        devname = os.path.join("/dev/", msgbufalias)
        self.assertRaises(OSError, os.stat, devname)
        # now configure alias and thus activate device
        self.wfile.seek(msgbuf * self.itemsize);
        self.wfile.write(struct.pack("<BQ55s", msgbuf, id, msgbufalias))
        # verify there is a device now
        self.assertTrue(stat.S_ISCHR(os.stat(devname).st_mode))
        # read back alias for verification
        self.rfile.seek(msgbuf * self.itemsize)
        rmsgbuf, rid, rmsgbufalias = struct.unpack("<BQ55s", self.rfile.read(self.itemsize))
        self.assertEqual(msgbuf, rmsgbuf)
        self.assertEqual(id, rid)
        self.assertEqual(msgbufalias, rmsgbufalias.rstrip('\x00'))

        # test open/close
        mb7 = io.FileIO(os.path.join("/dev/", msgbufalias), 'w')
        mb7.close();
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")

class ACMTestMsgBufDesc(ACMBaseTests.ACMTest):
    items = 32
    itemsize = 4
    defaultValue = 0
    filename = "config_bin/msg_buff_desc"

    def test_read_arbitrary_entry(self):
        msgbufsize= 1536
        msgbufoffs = 2048
        testval = (((msgbufsize / 4) - 1) << 21) | msgbufoffs
        msgbuf = random.randint(0, self.items - 1)
        # since we read from cache we have to write it via sysfs
        self.wfile.seek(msgbuf * self.itemsize);
        self.wfile.write(struct.pack(fmt[self.itemsize], testval))
        self.rfile.seek(msgbuf * self.itemsize);
        self.assertEqual(struct.unpack(fmt[self.itemsize], self.rfile.read(self.itemsize))[0], testval, 'message buffer={0}'.format(msgbuf))
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")

    def test_write_arbitrary_entry(self):
        msgbufsize= 1536
        msgbufoffs = 2048
        testval= (((msgbufsize / 4) - 1) << 21) + msgbufoffs
        msgbuf = random.randint(0, self.items - 1)
        self.wfile.seek(msgbuf * self.itemsize);
        self.wfile.write(struct.pack(fmt[self.itemsize], testval))
        self.assertEqual(AddressMap["Messagebuffer"].read(msgbuf * self.itemsize, self.itemsize)[0], testval, 'message buffer={0}'.format(msgbuf))
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")


class ACMTestPrefetchDMA(ACMBaseTests.ACMTest):
    items_per_bypass = 0x100
    items = ACMBaseTests.ACMTest.nr_bypass * items_per_bypass
    itemsize = 4
    itemsize_sysfs = itemsize
    itemoffs = itemsize
    defaultValue = 0
    filename = "config_bin/prefetch_dma"

    def test_read_entry_with_arbitrary_values(self):
        itemnr = random.randint(0, self.items - 1)
        itemdata = bytearray((random.getrandbits(8) for i in xrange(self.itemsize_sysfs)))
        
        for offs in range(0, self.itemsize, 4):
            AddressMap["Bypass0"].write(33 * self.itemoffs + 0x5800 + offs,
                                        struct.unpack("<I", itemdata[offs:offs+4]))
        self.rfile.seek(33 * self.itemsize_sysfs);
        rdata = self.rfile.read(self.itemsize_sysfs)
        # ignore padding data
        self.assertEqual(itemdata[0:self.itemsize], rdata)

        for offs in range(0, self.itemsize, 4):
            AddressMap["Bypass1"].write(10 * self.itemoffs + 0x5800 + offs,
                                        struct.unpack("<I", itemdata[offs:offs+4]))
        self.rfile.seek((10 + self.items_per_bypass) * self.itemsize_sysfs);
        rdata = self.rfile.read(self.itemsize_sysfs)
        # ignore padding data
        self.assertEqual(itemdata[0:self.itemsize], rdata)
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")


    def test_write_entry_with_arbitrary_values(self):
        itemnr = random.randint(0, self.items - 1)
        itemdata = bytearray((random.getrandbits(8) for i in xrange(self.itemsize_sysfs)))

        self.wfile.seek(45 * self.itemsize_sysfs);
        self.wfile.write(itemdata)
        rdata = AddressMap["Bypass0"].read(45 * self.itemoffs + 0x5800, self.itemsize / 4)
        for offs in range(0, self.itemsize, 4):
            self.assertEqual(itemdata[offs:offs+4], struct.pack("<I",rdata[offs / 4 ]))
 
        self.wfile.seek((self.items_per_bypass + 20) * self.itemsize_sysfs);
        self.wfile.write(itemdata)
        rdata = AddressMap["Bypass1"].read(20 * self.itemoffs + 0x5800, self.itemsize / 4)
        for offs in range(0, self.itemsize, 4):
            self.assertEqual(itemdata[offs:offs+4], struct.pack("<I",rdata[offs / 4 ]))
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")



    def test_wrong_size_alignment(self):
        super(ACMTestPrefetchDMA, self).test_wrong_size_alignment()


class ACMTestRedundancyCtrlTable(ACMBaseTests.ACMTest):
    items_per_table = 32
    items = ACMBaseTests.ACMTest.nr_bypass * items_per_table
    itemsize = 4
    itemsize_sysfs = itemsize;
    itemoffs = itemsize
    defaultValue = 0
    valMask = 0b00000000000111110100001100010111
    filename = "config_bin/redund_cnt_tab"


    def test_read_all_items_sequentially(self):
        super(ACMTestRedundancyCtrlTable, self).test_read_all_items_sequentially()
        
    def test_read_entire_table(self):
        for i in range(0, 2):
            self.rfile.seek(i * self.itemsize * self.items / 2)
            data = self.rfile.read(self.itemsize * self.items / 2)
            for j in range(0, self.items / 2):
                self.assertEqual(struct.unpack(fmt[self.itemsize], data[j:j+4])[0], 0)
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")

    def test_read_entry_with_predefined_value(self):
        itemnr = random.randint(0, self.items - 1)
        if compatible == 'ttt,acm-3.0':
            itemdata = bytearray(b'\x07\x43\x1f\x00')
        else:
            itemdata = bytearray(b'\x17\x43\x1f\x00')
        
        for offs in range(0, self.itemsize, 4):
            AddressMap["Redundancy"].write(15 * self.itemoffs + 0x200 + offs,
                                        struct.unpack("<I", itemdata[offs:offs+4]))
        self.rfile.seek(15 * self.itemsize_sysfs);
        rdata = self.rfile.read(self.itemsize_sysfs)
        # ignore padding data
        self.assertEqual(itemdata[0:self.itemsize], rdata[0:self.itemsize])

        for offs in range(0, self.itemsize, 4):
            AddressMap["Redundancy"].write(10 * self.itemoffs + 0x300 + offs,
                                        struct.unpack("<I", itemdata[offs:offs+4]))
        self.rfile.seek((10 + self.items_per_table) * self.itemsize_sysfs);
        rdata = self.rfile.read(self.itemsize_sysfs)
        # ignore padding data
        self.assertEqual(itemdata[0:self.itemsize], rdata)
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")

    def test_write_entry_with_predefined_value(self):
        # IP < 1.0.0 (?): itemdata = bytearray(b'\x17\x43\x1f\x00')
        if compatible == 'ttt,acm-3.0':
            itemdata = bytearray(b'\x07\x43\x1f\x00')
        else:
            itemdata = bytearray(b'\x17\x43\x1f\x00')

        self.wfile.seek(3 * self.itemsize_sysfs);
        self.wfile.write(itemdata)
        rdata = AddressMap["Redundancy"].read(3 * self.itemoffs + 0x200, self.itemsize / 4)
        for offs in range(0, self.itemsize, 4):
            self.assertEqual(itemdata[offs:offs+4], struct.pack("<I",rdata[offs / 4 ]))

        self.wfile.seek((self.items_per_table + 20) * self.itemsize_sysfs);
        self.wfile.write(itemdata)
        rdata = AddressMap["Redundancy"].read(20 * self.itemoffs + 0x300, self.itemsize / 4)
        for offs in range(0, self.itemsize, 4):
            self.assertEqual(itemdata[offs:offs+4], struct.pack("<I",rdata[offs / 4 ]))
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")

class ACMTestRedundancyStatusTable(ACMBaseTests.ACMTest):
    items = 32
    itemsize = 4
    itemsize_sysfs = itemsize;
    itemoffs = itemsize
    defaultValue = 0
    readonly = True
    filename = "config_bin/redund_status_tab"

class ACMTestRedundancyIntSeqNumTable(ACMBaseTests.ACMTest):
    items = 32
    itemsize = 4
    itemsize_sysfs = itemsize;
    itemoffs = itemsize
    defaultValue = 0
    filename = "config_bin/redund_intseqnum_tab"

    def test_read_entry_with_predefined_value(self):
        # it's a 16 bit data only, so keep upper 16 bits 0
        itemdata = bytearray(b'\x17\x43\x00\x00')

        for offs in range(0, self.itemsize, 4):
            AddressMap["Redundancy"].write(13 * self.itemoffs + 0x600 + offs,
                                        struct.unpack("<I", itemdata[offs:offs+4]))
        self.rfile.seek(13 * self.itemsize_sysfs);
        rdata = self.rfile.read(self.itemsize_sysfs)
        # ignore padding data
        self.assertEqual(itemdata[0:self.itemsize], rdata[0:self.itemsize])
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")

    def test_write_entry_with_predefined_value(self):
        # it's a 16 bit data only, so keep upper 16 bits 0
        itemdata = bytearray(b'\x34\xCB\x00\x00')

        self.wfile.seek(23 * self.itemsize_sysfs);
        self.wfile.write(itemdata)
        rdata = AddressMap["Redundancy"].read(23 * self.itemoffs + 0x600, self.itemsize / 4)
        for offs in range(0, self.itemsize, 4):
            self.assertEqual(itemdata[offs:offs+4], struct.pack("<I",rdata[offs / 4 ]))
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")


class ACMTestScatterDMA(ACMBaseTests.ACMTest):
    items_per_bypass = 0x100
    items = ACMBaseTests.ACMTest.nr_bypass * items_per_bypass
    itemsize = 4
    itemsize_sysfs = itemsize
    itemoffs = itemsize
    defaultValue = 0
    filename = "config_bin/scatter_dma"


    def test_wrong_size_alignment(self):
        super(ACMTestScatterDMA, self).test_wrong_size_alignment()

    def test_read_entry_with_arbitrary_values(self):
        itemnr = random.randint(0, self.items - 1)
        itemdata = bytearray((random.getrandbits(8) for i in xrange(self.itemsize_sysfs)))
        
        for offs in range(0, self.itemsize, 4):
            AddressMap["Bypass0"].write(33 * self.itemoffs + 0x3000 + offs,
                                        struct.unpack("<I", itemdata[offs:offs+4]))
        self.rfile.seek(33 * self.itemsize_sysfs);
        rdata = self.rfile.read(self.itemsize_sysfs)
        # ignore padding data
        self.assertEqual(itemdata[0:self.itemsize], rdata)

        for offs in range(0, self.itemsize, 4):
            AddressMap["Bypass1"].write(10 * self.itemoffs + 0x3000 + offs,
                                        struct.unpack("<I", itemdata[offs:offs+4]))
        self.rfile.seek((10 + self.items_per_bypass) * self.itemsize_sysfs);
        rdata = self.rfile.read(self.itemsize_sysfs)
        # ignore padding data
        self.assertEqual(itemdata[0:self.itemsize], rdata)
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")

    def test_write_entry_with_arbitrary_values(self):
        itemnr = random.randint(0, self.items - 1)
        itemdata = bytearray((random.getrandbits(8) for i in xrange(self.itemsize_sysfs)))

        self.wfile.seek(45 * self.itemsize_sysfs);
        self.wfile.write(itemdata)
        rdata = AddressMap["Bypass0"].read(45 * self.itemoffs + 0x3000, self.itemsize / 4)
        for offs in range(0, self.itemsize, 4):
            self.assertEqual(itemdata[offs:offs+4], struct.pack("<I",rdata[offs / 4 ]))
 
        self.wfile.seek((self.items_per_bypass + 20) * self.itemsize_sysfs);
        self.wfile.write(itemdata)
        rdata = AddressMap["Bypass1"].read(20 * self.itemoffs + 0x3000, self.itemsize / 4)
        for offs in range(0, self.itemsize, 4):
            self.assertEqual(itemdata[offs:offs+4], struct.pack("<I",rdata[offs / 4 ]))
        # make sure there is no kernel log
        self.assertEqual(Klog.readclear(), "")


class ACMTestSchedulerCycleTime(ACMBaseTests.ACMTest):
    items = 4
    itemsize = 8
    defaultValue = 0x5000000000L # 80ns after reset
    filename = "config_bin/sched_cycle_time"


class ACMTestSchedulerDownCounter(ACMBaseTests.ACMTest):
    items = 2
    itemsize = 2
    defaultValue = 0
    filename = "config_bin/sched_down_counter"


# start times are not constant any more
#class ACMTestSchedulerStartTime(ACMBaseTests.ACMTest):
#    items = 4
#    itemsize = 8
#    defaultValue = 0
#    filename = "config_bin/sched_start_table"

# scheduler table is used for schedule stop so it is not constant any more
#class ACMTestSchedulerTableRow(ACMBaseTests.ACMTest):
#    items = 4 * 1024
#    itemsize = 8
#    defaultValue = 0
#    filename = "config_bin/sched_tab_row"


class ACMTestStreamTrigger(ACMBaseTests.ACMTest):
    items = 2 * (0x10 + 1)
    itemsize = 4
    defaultValue = 0
    filename = "config_bin/stream_trigger"


    def test_wrong_size_alignment(self):
        super(ACMTestStreamTrigger, self).test_wrong_size_alignment()


class ACMTestTableStatus(ACMBaseTests.ACMTest):
    items = 4
    itemsize = 2
    defaultValue = 0x300
    defaultValueMask = ~0x2
    filename = "config_bin/table_status"
    readonly = True


class ACMTestLockMsgBuf(ACMBaseTests.ACMTest):
    items = 1
    itemsize = 4
    defaultValue = 0
    filename = "control_bin/lock_msg_bufs"


class ACMTestUnlockMsgBuf(ACMBaseTests.ACMTest):
    items = 1
    itemsize = 4
    defaultValue = 0xFFFFFFFF
    filename = "control_bin/unlock_msg_bufs"


class ACMTestIfcVersion(ACMBaseTests.ACMTestDual):
        defaultValue = "0X00000003"
        filename = "status/ifc_version"
        compatibleList = "ttt,acm-1.0"


class ACMTestConfigVersion(ACMBaseTests.ACMTestDual):
        defaultValue = "0X00000000"
        filename = "status/config_version"
        compatibleList = "ttt,acm-1.0"


class ACMTestDisableOverrunPrev(ACMBaseTests.ACMTestDual):
        defaultValue = "0X00"
        filename = "status/disable_overrun_prev"


class ACMTestDropFramesCntPrev(ACMBaseTests.ACMTestDual):
        defaultValue = "0X00"
        filename = "status/drop_frames_cnt_prev"


class ACMTestGMIIErrorsSetPrev(ACMBaseTests.ACMTestDual):
        defaultValue = "0X00"
        filename = "status/gmii_errors_set_prev"
        compatibleList = ["ttt,acm-2.0"]


class ACMTestGMIIErrorPrevCycle(ACMBaseTests.ACMTestDual):
        defaultValue = "0X00"
        filename = "status/gmii_error_prev_cycle"
        compatibleList = ["ttt,acm-3.0"]


class ACMTestLayer7MissmatchCnt(ACMBaseTests.ACMTestDual):
        defaultValue = "0X00"
        filename = "status/layer7_missmatch_cnt"


class ACMTestMIIErrors(ACMBaseTests.ACMTestDual):
        defaultValue = "0X00"
        filename = "status/mii_errors"


class ACMTestRuntFrames(ACMBaseTests.ACMTestDual):
        defaultValue = "0X00"
        filename = "status/runt_frames"


class ACMTestScatterDMAFramesCntPrev(ACMBaseTests.ACMTestDual):
        defaultValue = "0X0000"
        filename = "status/scatter_DMA_frames_cnt_prev"


class ACMTestTxFrameCycleChange(ACMBaseTests.ACMTestDual):
        defaultValue = "0X0000"
        filename = "status/tx_frame_cycle_change"


class ACMTestTxFramesPrev(ACMBaseTests.ACMTestDual):
        defaultValue = "0X0000"
        filename = "status/tx_frames_prev"


class ACMTestErrorFlags(ACMBaseTests.ACMTestDual):
        defaultValue = "0X000000000"
        filename = "error/error_flags"


class ACMTestHaltOnError(ACMBaseTests.ACMTestDual):
        defaultValue = "0"
        filename = "error/halt_on_error"


class ACMTestHaltOnOtherBypass(ACMBaseTests.ACMTestDual):
        defaultValue = "0"
        filename = "error/halt_on_other_bypass"


class ACMTestPolicingFlags(ACMBaseTests.ACMTestDual):
        defaultValue = "0X0"
        filename = "error/policing_flags"


class ACMTestRxBytes(ACMBaseTests.ACMTestDualDebug):
        defaultValue = "0X00"
        filename = "status/rx_bytes"


class ACMTestRxFrames(ACMBaseTests.ACMTestDualDebug):
        defaultValue = "0X00"
        filename = "status/rx_frames"


class ACMTestFcsErrors(ACMBaseTests.ACMTestDualDebug):
        defaultValue = "0X00"
        filename = "status/fcs_errors"


class ACMTestSizeErrors(ACMBaseTests.ACMTestDualDebug):
        defaultValue = "0X00"
        filename = "status/size_errors"


class ACMTestLookupMatchVector(ACMBaseTests.ACMTestDualDebug):
        defaultValue = "0X00"
        filename = "status/lookup_match_vec"


class ACMTestLayer7MatchVector(ACMBaseTests.ACMTestDualDebug):
        defaultValue = "0X00"
        filename = "status/layer7_match_vec"


class ACMTestLastStreamTrigger(ACMBaseTests.ACMTestDualDebug):
        defaultValue = "0X00"
        filename = "status/last_stream_trigger"


class ACMTestIngressWindowStatus(ACMBaseTests.ACMTestDualDebug):
        defaultValue = "0X00"
        filename = "status/ingress_win_stat"


class ACMTestSchedulerTriggerCountPreviousCycle(ACMBaseTests.ACMTestDualDebug):
        defaultValue = "0X00"
        filename = "status/sched_trig_cnt_prev_cyc"


class ACMTestSchedulerFirstConditionTriggerCountPreviousCycle(ACMBaseTests.ACMTestDualDebug):
        defaultValue = "0X00"
        filename = "status/sched_1st_cond_trig_cnt_prev_cyc"


class ACMTestPendingRequestMaxNum(ACMBaseTests.ACMTestDualDebug):
        defaultValue = "0X00"
        filename = "status/pending_req_max_num"


class ACMTestScatterDMAFramesCountCurrent(ACMBaseTests.ACMTestDualDebug):
        defaultValue = "0X00"
        filename = "status/scatter_DMA_frames_cnt_curr"


class ACMTestScatterDMABytesCountPrevious(ACMBaseTests.ACMTestDualDebug):
        defaultValue = "0X00"
        filename = "status/scatter_DMA_bytes_cnt_prev"


class ACMTestScatterDMABytesCountCurrent(ACMBaseTests.ACMTestDualDebug):
        defaultValue = "0X00"
        filename = "status/scatter_DMA_bytes_cnt_curr"


class ACMTestTxBytesPrevious(ACMBaseTests.ACMTestDualDebug):
        defaultValue = "0X00"
        filename = "status/tx_bytes_prev"


class ACMTestTxBytesCurrent(ACMBaseTests.ACMTestDualDebug):
        defaultValue = "0X00"
        filename = "status/tx_bytes_curr"


class ACMTestTxFramesCycleFirstChange(ACMBaseTests.ACMTestDualDebug):
        defaultValue = "0X00"
        filename = "status/tx_frames_cyc_1st_change"


class ACMTestTxFramesCycleLastChange(ACMBaseTests.ACMTestDualDebug):
        defaultValue = "0X00"
        filename = "status/tx_frames_cyc_last_change"


class ACMTestRxFramesCriticalCurrent(ACMBaseTests.ACMTestDualDebug):
        defaultValue = "0X00"
        filename = "status/rx_frames_critical_curr"
        incompatibleList = ['ttt,acm-3.0']

class ACMTestRxFramesCurrent(ACMBaseTests.ACMTestDualDebug):
        defaultValue = "0X00"
        filename = "status/rx_frames_curr"
        compatibleList = ['ttt,acm-3.0']


#
# DevMem derived  from https://github.com/kylemanna/pydevmem
#
""" DevMemBuffer
This class holds data for objects returned from DevMem class
It allows an easy way to print hex data
"""
class DevMemBuffer:
 
    def __init__(self, base_addr, data):
        self.data = data
        self.base_addr = base_addr
 
    def __len__(self):
        return len(self.data)
 
    def __getitem__(self, key):
        return self.data[key]
 
    def __setitem__(self, key, value):
        self.data[key] = value
 
    def hexdump(self, word_size = 4, words_per_row = 4):
        # Build a list of strings and then join them in the last step.
        # This is more efficient then concat'ing immutable strings.
 
        d = self.data
        dump = []
 
        word = 0
        while (word < len(d)):
            dump.append('0x{0:02x}:  '.format(self.base_addr
                                              + word_size * word))
 
            max_col = word + words_per_row
            if max_col > len(d): max_col = len(d)
 
            while (word < max_col):
                # If the word is 4 bytes, then handle it and continue the
                # loop, this should be the normal case
                if word_size == 4:
                    dump.append(" {0:08x} ".format(d[word]))
                    word += 1
                    continue
 
                # Otherwise the word_size is not an int, pack it so it can be
                # un-packed to the desired word size.  This should blindly
                # handle endian problems (Verify?)
                packed = struct.pack('I',(d[word]))
                if word_size == 2:
                    dh = struct.unpack('HH', packed)
                    dump.append(" {0:04x}".format(dh[0]))
                    word += 1
                elif word_size == 1:
                    db = struct.unpack('BBBB', packed)
                    dump.append(" {0:02x}".format(db[0]))
                    word += 1
 
            dump.append('\n')
 
        # Chop off the last new line character and join the list of strings
        # in to a single string
        return ''.join(dump[:-1])
 
    def __str__(self):
        return self.hexdump()
 
class DevMem:
    # word: Size of a word that will be used for reading/writing
    def __init__(self, base_addr, length = 1, word = 4, filename = '/dev/mem',
                 debug = 0):
 
        if base_addr < 0 or length < 0: raise AssertionError
        self.word = word
        self.mask = ~(word - 1);
        self._debug = debug
        if word ==  8:
            self.packfmt = 'Q'
        elif word ==  4:
            self.packfmt = 'I'
        elif word == 2:
            self.packfmt = 'H'
        elif word == 1:
            self.packfmt = 'B'
        else:
            print "DevMem: wrong word size"
            sys.exit(1)
 
        self.base_addr = base_addr & ~(mmap.PAGESIZE - 1)
        self.base_addr_offset = base_addr - self.base_addr
 
        stop = base_addr + length * self.word
        if (stop % self.mask):
            stop = (stop + self.word) & ~(self.word - 1)
 
        self.length = stop - self.base_addr
        self.fname = filename
 
        self.debug('init with base_addr = {0} and length = {1} on {2}'.
                format(hex(self.base_addr), hex(self.length), self.fname))
 
        # Open file and mmap
        f = os.open(self.fname, os.O_RDWR | os.O_SYNC)
        self.mem = mmap.mmap(f, self.length, mmap.MAP_SHARED,
                mmap.PROT_READ | mmap.PROT_WRITE,
                offset=self.base_addr)
 
 
    """
    Read length number of words from offset
    """
    def read(self, offset, length):
        if offset < 0 or length < 0: raise AssertionError
 
        # Make reading easier (and faster... won't resolve dot in loops)
        mem = self.mem
 
        self.debug('reading {0} bytes from offset {1}'.
                   format(length * self.word, hex(offset)))
 
        # Compensate for the base_address not being what the user requested
        # and then seek to the aligned offset.
        virt_base_addr = self.base_addr_offset & self.mask
        mem.seek(virt_base_addr + offset)
 
        # Read length words of size self.word and return it
        data = []
        for i in range(length):
            data.append(struct.unpack(self.packfmt, mem.read(self.word))[0])
 
        abs_addr = self.base_addr + virt_base_addr
        return DevMemBuffer(abs_addr + offset, data)
 
 
    """
    Write length number of words to offset
    """
    def write(self, offset, din):
        if offset < 0 or len(din) <= 0: raise AssertionError
 
        self.debug('writing {0} bytes to offset {1}'.
                format(len(din), hex(offset)))
 
        # Make reading easier (and faster... won't resolve dot in loops)
        mem = self.mem
 
        # Compensate for the base_address not being what the user requested
        offset += self.base_addr_offset
 
        # Check that the operation is going write to an aligned location
        if (offset & ~self.mask): raise AssertionError
 
        # Seek to the aligned offset
        virt_base_addr = self.base_addr_offset & self.mask
        mem.seek(virt_base_addr + offset)
 
        # Read until the end of our aligned address
        for i in range(0, len(din), self.word):
            self.debug('writing at position = {0}: 0x{1:x}'.
                        format(self.mem.tell(), din[i]))
            # Write one word at a time
            mem.write(struct.pack(self.packfmt, din[i]))
 
    def debug_set(self, value):
        self._debug = value
 
    def debug(self, debug_str):
        if self._debug: print('DevMem Debug: {0}'.format(debug_str))


class Klog:
    @staticmethod
    def clear():
        subprocess.Popen(["dmesg", "--console-off"]).wait()
        subprocess.Popen(["dmesg", "--clear"]).wait()
    @staticmethod
    def readclear():
        p = subprocess.Popen(["dmesg", "--level", "info,notice,warn,err,crit,alert,emerg"], stdout=subprocess.PIPE)
        p.wait()
        buffer = p.communicate()[0]
        subprocess.Popen(["dmesg", "--console-on"]).wait()
        return buffer

def readString(myfile):
    chars = []
    while True:
        c = myfile.read(1)
        if c == "":
            return ""
        if c == chr(0):
            return "".join(chars)
        chars.append(c)

if __name__ == '__main__':

    if ACMBaseTests.detectDriver("ngn_dev"):
        oldDriver = True
        print "*** Old ngn_dev driver detected ***"
        AddressMapConfig = {"Bypass0":        [0xc0030000, 0x10000, 4], 
                            "Bypass1":        [0xc0020000, 0x10000, 4],
                            "Redundancy":     [0xc00a0000, 0x10000, 4],
                            "Messagebuffer":  [0xc0040000, 0x40000, 4],
                            "Scheduler":      [0xc0080000, 0x20000, 2],
                            }
    else:
        oldDriver = False
    if ACMBaseTests.detectDriver("acm"):
        newDriver = True
        compatfile = io.FileIO('/sys/firmware/devicetree/base/soc/ngn/compatible', 'r')
        compatible = compatfile.read().strip(chr(0))
        compatfile.close()
        # read address map from devicetree
        AddressMapConfig = {}
        regname_list = []
        with open('/sys/firmware/devicetree/base/soc/ngn/reg-names','rb') as regnames:
            while True:
                regname = readString(regnames)
                if len(regname) == 0:
                    break
                regname_list.append(regname)
            regnames.close()
        with open('/sys/firmware/devicetree/base/soc/ngn/reg','rb') as regs:
            print "Addressmap from devicetree:"
            for i in range(len(regname_list)):
                be = regs.read(4)
                start = struct.unpack('>L', be)[0]
                be = regs.read(4)
                size = struct.unpack('>L', be)[0]
                if regname_list[i] == 'Scheduler':
                    width = 2
                else:
                    width = 4
                AddressMapConfig[regname_list[i]] = [start, size , width]
                print "  0x%08x - 0x%08x: %s" % (start, start + size - 1, regname_list[i])
            regs.close()
        print "*** New acm driver detected: %s ***" % compatible
    else:
        newDriver = False

    if not oldDriver and not newDriver:
        print "No acm/ngn driver loaded, aborting"
        sys.exit(1)
    if oldDriver and newDriver:
        print "Amibigous driver detection, aborting"
        sys.exit(1)
    
    # map all memory areas
    AddressMap = {}
    for mapname in AddressMapConfig:
       AddressMap[mapname] = DevMem(AddressMapConfig[mapname][0],
                                    AddressMapConfig[mapname][1],
                                    AddressMapConfig[mapname][2])

    if compatible != 'ttt,acm-1.0':
        device_id = AddressMap['CommonRegister'].read(0, 1)[0]
        version_id = AddressMap['CommonRegister'].read(4, 1)[0]
        revision_id = AddressMap['CommonRegister'].read(8, 1)[0]
        major = (version_id & 0xFF000000) >> 24
        minor = (version_id & 0x00FF0000) >> 16
        patch = (version_id & 0x0000FF00) >> 8
        debugIP = AddressMap['CommonRegister'].read(0x100, 1)[0] & 0x00000001
        print "ACM IP %s Version %d.%d.%d (%s), debug=%d" % (hex(device_id), major, minor, patch, format(revision_id, 'x'), debugIP)
    fcs_ids = AddressMap["Scheduler"].read(0, 4)
    print "Scheduler IP identification: 0x%08x " % (fcs_ids[0] + (fcs_ids[1] << 16))
    print "          IP Version: 0x%08x " % (fcs_ids[2] + (fcs_ids[3] << 16))

    unittest.main()
