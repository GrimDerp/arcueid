package org.arcueid_arc.nekoarc.vm;

import static org.junit.Assert.*;

import org.arcueid_arc.nekoarc.NekoArcException;
import org.arcueid_arc.nekoarc.Nil;
import org.arcueid_arc.nekoarc.types.Fixnum;
import org.junit.Test;

public class VirtualMachineTest {

	@Test
	public void testHLT() throws NekoArcException
	{
		byte inst[] = { 0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(1234, ((Fixnum)vm.getAcc()).fixnum);
		assertEquals(1, vm.getIp());
	}

	@Test
	public void testNOP() throws NekoArcException
	{
		byte inst[] = { 0x00, 0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(1234, ((Fixnum)vm.getAcc()).fixnum);
		assertEquals(2, vm.getIp());
	}

	@Test
	public void testNIL() throws NekoArcException
	{
		byte inst[] = { 0x13, 0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Nil.NIL, vm.getAcc());
		assertEquals(2, vm.getIp());
	}
}