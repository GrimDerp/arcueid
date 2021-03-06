package org.arcueidarc.nekoarc.vm;

import org.arcueidarc.nekoarc.HeapEnv;
import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.Unbound;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.types.Symbol;
import org.arcueidarc.nekoarc.util.ObjectMap;
import org.arcueidarc.nekoarc.vm.instruction.*;

public class VirtualMachine
{
	private int sp;					// stack pointer
	private ArcObject env;			// environment pointer
	private ArcObject cont;			// continuation pointer
	private ArcObject[] stack;		// stack
	private int ip;					// instruction pointer
	private byte[] code;
	private boolean runnable;
	private ArcObject acc;			// accumulator
	private ArcObject[] literals;
	private int argc;				// argument counter for current function
	private static final INVALID NOINST = new INVALID();
	private static final Instruction[] jmptbl = {
		new NOP(),		// 0x00
		new PUSH(),		// 0x01
		new POP(),		// 0x02
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		new RET(),		// 0x0d
		NOINST,
		NOINST,
		NOINST,
		new NO(),		// 0x11
		new TRUE(),		// 0x12
		new NIL(),		// 0x13
		new HLT(),		// 0x14
		new ADD(),		// 0x15
		new SUB(),		// 0x16
		new MUL(),		// 0x17
		new DIV(),		// 0x18
		new CONS(),		// 0x19
		new CAR(),		// 0x1a
		new CDR(),		// 0x1b
		new SCAR(),		// 0x1c
		new SCDR(),		// 0x1d
		NOINST,
		new IS(),		// 0x1f
		NOINST,
		NOINST,
		new DUP(),		// 0x22
		NOINST,			// 0x23
		new CONSR(),		// 0x24
		NOINST,
		new DCAR(),		// 0x26
		new DCDR(),		// 0x27
		new SPL(),		// 0x28
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		new LDL(),		// 0x43
		new LDI(),		// 0x44
		new LDG(),		// 0x45
		new STG(),		// 0x46
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		new APPLY(),		// 0x4c
		new CLS(),		// 0x4d,
		new JMP(),		// 0x4e
		new JT(),		// 0x4f
		new JF(),		// 0x50
		new JBND(),		// 0x51
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		new MENV(),		// 0x65
		NOINST,
		NOINST,
		NOINST,
		new LDE0(),		// 0x69
		new STE0(),		// 0x6a
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		new LDE(),		// 0x87
		new STE(),		// 0x88
		new CONT(),		// 0x89
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		new ENV(),		// 0xca
		new ENVR(),		// 0xcb
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
	};

	private ObjectMap<Symbol, ArcObject> genv = new ObjectMap<Symbol, ArcObject>();

	public VirtualMachine(int stacksize)
	{
		sp = 0;
		stack = new ArcObject[stacksize];
		ip = 0;
		code = null;
		runnable = true;
		env = Nil.NIL;
		cont = Nil.NIL;
		setAcc(Nil.NIL);
	}

	public void load(final byte[] instructions, int ip, final ArcObject[] literals)
	{
		this.code = instructions;
		this.literals = literals;
		this.ip = ip;
	}

	public void load(final byte[] instructions, int ip)
	{
		load(instructions, ip, null);
	}

	public void halt()
	{
		runnable = false;
	}

	public void nextI()
	{
		ip++;
	}

	/**
	 * Attempt to garbage collect the stack, after Richard A. Kelsey, "Tail Recursive Stack Disciplines for an Interpreter"
	 * Basically, the only things on the stack that matter are environments and continuations.
	 * The algorithm here works by copying all environments and continuations from the stack to the heap. When that's done it will move the stack pointer
	 * to the top of the stack.
	 * 1. Start with the environment register. Move that environment and all of its children to the heap.
	 * 2. Continue with the continuation register. Move all those continuations and all their children to the heap.
	 * 3. Move the remaining stack elements
	 */
	private void stackgc()
	{
	}

	public void push(ArcObject obj)
	{
		for (;;) {
			try {
				stack[sp++] = obj;
				return;
			} catch (ArrayIndexOutOfBoundsException e) {
				// We can try to garbage collect the stack
				stackgc();
				if (sp >= stack.length)
					throw new NekoArcException("stack overflow");
			}
		}
	}

	public ArcObject pop()
	{
		return(stack[--sp]);
	}

	public void run()
		throws NekoArcException
	{
		while (runnable)
			jmptbl[(int)code[ip++] & 0xff].invoke(this);
	}

	// Four-byte instruction arguments (most everything else). Little endian.
	public int instArg()
	{
		long val = 0;
		int data;
		for (int i=0; i<4; i++) {
			data = (((int)code[ip++]) & 0xff);
			val |= data << i*8;
		}
		return((int)((val << 1) >> 1));
	}

	// one-byte instruction arguments (LDE/STE/ENV, etc.)
	public byte smallInstArg()
	{
		return(code[ip++]);
	}

	public ArcObject getAcc()
	{
		return acc;
	}

	public void setAcc(ArcObject acc)
	{
		this.acc = acc;
	}

	public boolean runnable()
	{
		return(this.runnable);
	}

	public ArcObject literal(int offset)
	{
		return(literals[offset]);
	}

	public int getIP()
	{
		return ip;
	}

	public void setIP(int ip)
	{
		this.ip = ip;
	}

	public int getSP()
	{
		return(sp);
	}

	// add or replace a global binding
	public ArcObject bind(Symbol sym, ArcObject binding)
	{
		genv.put(sym, binding);
		return(binding);
	}

	public ArcObject value(Symbol sym)
	{
		if (!genv.containsKey(sym))
			throw new NekoArcException("Unbound symbol " + sym);
		return(genv.get(sym));
	}

	public int argc()
	{
		return(argc);
	}

	public int setargc(int ac)
	{
		return(argc = ac);
	}

	public void argcheck(int minarg, int maxarg)
	{
		if (argc() < minarg)
			throw new NekoArcException("too few arguments, at least " + minarg + " required, " + argc() + " passed");
		if (maxarg >= 0 && argc() > maxarg)
			throw new NekoArcException("too many arguments, at most " + maxarg + " allowed, " + argc() + " passed");
	}

	public void argcheck(int arg)
	{
		argcheck(arg, arg);
	}

	/* Create a stack-based environment. */
	public void mkenv(int prevsize, int extrasize)
	{
		// Add the extra environment entries
		for (int i=0; i<extrasize; i++)
			push(Unbound.UNBOUND);
		int count = prevsize + extrasize;
		int envstart = sp - count;

		/* Stack environments are basically Fixnum pointers into the stack. */
		int envptr = sp;
		push(Fixnum.get(envstart));		// envptr
		push(Fixnum.get(count));		// envptr + 1
		push(env);						// envptr + 2
		env = Fixnum.get(envptr);
	}

	/** move current environment to heap if needed */
	public ArcObject heapenv()
	{
		if (env instanceof Fixnum)
			env = HeapEnv.fromStackEnv(this, (int)((Fixnum)env).fixnum);
		return(env);
	}

	private ArcObject findenv(int depth)
	{
		ArcObject cenv = env;

		while (depth-- > 0 && !cenv.is(Nil.NIL)) {
			if (cenv instanceof Fixnum) {
				int index = (int)((Fixnum)cenv).fixnum;
				cenv = stackIndex(index+2);
			} else {
				HeapEnv e = (HeapEnv)cenv;
				cenv = e.prevEnv();
			}
		}
		return(cenv);
	}

	public ArcObject getenv(int depth, int index)
	{
		ArcObject cenv = findenv(depth);
		if (cenv == Nil.NIL)
			throw new NekoArcException("environment depth exceeded");
		if (cenv instanceof Fixnum) {
			int si = (int)((Fixnum)cenv).fixnum;
			int start = (int)((Fixnum)stackIndex(si)).fixnum;
			int size = (int)((Fixnum)stackIndex(si+1)).fixnum;
			if (index > size)
				throw new NekoArcException("stack environment index exceeded");
			return(stackIndex(start+index));
		}
		return(((HeapEnv)cenv).getEnv(index));
	}

	public ArcObject setenv(int depth, int index, ArcObject value)
	{
		ArcObject cenv = findenv(depth);
		if (cenv == Nil.NIL)
			throw new NekoArcException("environment depth exceeded");
		if (cenv instanceof Fixnum) {
			int si = (int)((Fixnum)cenv).fixnum;
			int start = (int)((Fixnum)stackIndex(si)).fixnum;
			int size = (int)((Fixnum)stackIndex(si+1)).fixnum;
			if (index > size)
				throw new NekoArcException("stack environment index exceeded");
			return(setStackIndex(start+index, value));
		}
		return(((HeapEnv)cenv).setEnv(index,value));
	}

	public ArcObject stackIndex(int index)
	{
		return(stack[index]);
	}

	public ArcObject setStackIndex(int index, ArcObject value)
	{
		return(stack[index] = value);
	}

	// Make a continuation on the stack. The new continuation is saved in the continuation register.
	public void makecont(int ipoffset)
	{
		int newip = ip + ipoffset;
		push(Fixnum.get(newip));
		push(env);
		push(cont);
		cont = Fixnum.get(sp);
	}

	// Restore continuation
	public void restorecont()
	{
		if (cont instanceof Fixnum) {
			sp = (int)((Fixnum)cont).fixnum;
			cont = pop();
			setEnv(pop());
			setIP((int)((Fixnum)pop()).fixnum);
		} else if (cont.is(Nil.NIL)) {
			// If we have no continuation that was an attempt to return from the topmost
			// level and we should halt the machine.
			halt();
		} else {
			throw new NekoArcException("invalid continuation");
		}
	}

	public void setEnv(ArcObject env)
	{
		this.env = env; 
	}
}
