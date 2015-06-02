package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class STE0 implements Instruction
{
	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		int indx = vm.instArg();
		vm.setenv(0, indx, vm.getAcc());
	}

}
