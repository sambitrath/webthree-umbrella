/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * @author Christian <c@ethdev.com>
 * @date 2014
 * Utilities for the solidity compiler.
 */

#pragma once

#include <ostream>
#include <libevmcore/Instruction.h>
#include <libevmcore/Assembly.h>
#include <libsolidity/ASTForward.h>
#include <libsolidity/Types.h>

namespace dev {
namespace solidity {


/**
 * Context to be shared by all units that compile the same contract.
 * It stores the generated bytecode and the position of identifiers in memory and on the stack.
 */
class CompilerContext
{
public:
	void addMagicGlobal(MagicVariableDeclaration const& _declaration);
	void addStateVariable(VariableDeclaration const& _declaration);
	void startNewFunction() { m_localVariables.clear(); m_asm.setDeposit(0); }
	void addVariable(VariableDeclaration const& _declaration);
	void addAndInitializeVariable(VariableDeclaration const& _declaration);
	void addFunction(FunctionDefinition const& _function);

	void setCompiledContracts(std::map<ContractDefinition const*, bytes const*> const& _contracts) { m_compiledContracts = _contracts; }
	bytes const& getCompiledContract(ContractDefinition const& _contract) const;

	void adjustStackOffset(int _adjustment) { m_asm.adjustDeposit(_adjustment); }

	bool isMagicGlobal(Declaration const* _declaration) const { return m_magicGlobals.count(_declaration); }
	bool isFunctionDefinition(Declaration const* _declaration) const { return m_functionEntryLabels.count(_declaration); }
	bool isLocalVariable(Declaration const* _declaration) const;
	bool isStateVariable(Declaration const* _declaration) const { return m_stateVariables.count(_declaration); }

	eth::AssemblyItem getFunctionEntryLabel(FunctionDefinition const& _function) const;
	/// Returns the distance of the given local variable from the top of the local variable stack.
	unsigned getBaseStackOffsetOfVariable(Declaration const& _declaration) const;
	/// If supplied by a value returned by @ref getBaseStackOffsetOfVariable(variable), returns
	/// the distance of that variable from the current top of the stack.
	unsigned baseToCurrentStackOffset(unsigned _baseOffset) const;
	u256 getStorageLocationOfVariable(Declaration const& _declaration) const;

	/// Appends a JUMPI instruction to a new tag and @returns the tag
	eth::AssemblyItem appendConditionalJump() { return m_asm.appendJumpI().tag(); }
	/// Appends a JUMPI instruction to @a _tag
	CompilerContext& appendConditionalJumpTo(eth::AssemblyItem const& _tag) { m_asm.appendJumpI(_tag); return *this; }
	/// Appends a JUMP to a new tag and @returns the tag
	eth::AssemblyItem appendJumpToNew() { return m_asm.appendJump().tag(); }
	/// Appends a JUMP to a tag already on the stack
	CompilerContext&  appendJump() { return *this << eth::Instruction::JUMP; }
	/// Appends a JUMP to a specific tag
	CompilerContext& appendJumpTo(eth::AssemblyItem const& _tag) { m_asm.appendJump(_tag); return *this; }
	/// Appends pushing of a new tag and @returns the new tag.
	eth::AssemblyItem pushNewTag() { return m_asm.append(m_asm.newPushTag()).tag(); }
	/// @returns a new tag without pushing any opcodes or data
	eth::AssemblyItem newTag() { return m_asm.newTag(); }
	/// Adds a subroutine to the code (in the data section) and pushes its size (via a tag)
	/// on the stack. @returns the assembly item corresponding to the pushed subroutine, i.e. its offset.
	eth::AssemblyItem addSubroutine(eth::Assembly const& _assembly) { return m_asm.appendSubSize(_assembly); }
	/// Pushes the size of the final program
	void appendProgramSize() { return m_asm.appendProgramSize(); }
	/// Adds data to the data section, pushes a reference to the stack
	eth::AssemblyItem appendData(bytes const& _data) { return m_asm.append(_data); }

	/// Append elements to the current instruction list and adjust @a m_stackOffset.
	CompilerContext& operator<<(eth::AssemblyItem const& _item) { m_asm.append(_item); return *this; }
	CompilerContext& operator<<(eth::Instruction _instruction) { m_asm.append(_instruction); return *this; }
	CompilerContext& operator<<(u256 const& _value) { m_asm.append(_value); return *this; }
	CompilerContext& operator<<(bytes const& _data) { m_asm.append(_data); return *this; }

	eth::Assembly const& getAssembly() const { return m_asm; }
	void streamAssembly(std::ostream& _stream) const { _stream << m_asm; }
	bytes getAssembledBytecode(bool _optimize = false) { return m_asm.optimise(_optimize).assemble(); }

private:
	eth::Assembly m_asm;

	/// Magic global variables like msg, tx or this, distinguished by type.
	std::set<Declaration const*> m_magicGlobals;
	/// Other already compiled contracts to be used in contract creation calls.
	std::map<ContractDefinition const*, bytes const*> m_compiledContracts;
	/// Size of the state variables, offset of next variable to be added.
	u256 m_stateVariablesSize = 0;
	/// Storage offsets of state variables
	std::map<Declaration const*, u256> m_stateVariables;
	/// Offsets of local variables on the stack (relative to stack base).
	std::map<Declaration const*, unsigned> m_localVariables;
	/// Sum of stack sizes of local variables
	unsigned m_localVariablesSize;
	/// Labels pointing to the entry points of funcitons.
	std::map<Declaration const*, eth::AssemblyItem> m_functionEntryLabels;
};

}
}