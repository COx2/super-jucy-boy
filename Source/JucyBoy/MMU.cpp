#include "MMU.h"
#include "Mbc1.h"
#include <fstream>
#include <cassert>

MMU::MMU()
{
	Reset();
}

MMU::~MMU()
{
	mbc_.reset();
}

void MMU::Reset()
{
	memory_.clear();

	memory_.emplace_back(Memory::GetSizeOfRegion(Memory::Region::ROM_Bank0), 0);
	memory_.emplace_back(Memory::GetSizeOfRegion(Memory::Region::ROM_OtherBanks), 0);
	memory_.emplace_back(Memory::GetSizeOfRegion(Memory::Region::VRAM), 0);
	memory_.emplace_back(Memory::GetSizeOfRegion(Memory::Region::ERAM), 0);
	memory_.emplace_back(Memory::GetSizeOfRegion(Memory::Region::WRAM), 0);
	memory_.emplace_back(Memory::GetSizeOfRegion(Memory::Region::WRAM_Echo), 0);
	memory_.emplace_back(Memory::GetSizeOfRegion(Memory::Region::OAM), 0);
	memory_.emplace_back(Memory::GetSizeOfRegion(Memory::Region::Unused), 0);
	memory_.emplace_back(Memory::GetSizeOfRegion(Memory::Region::IO), 0);
	memory_.emplace_back(Memory::GetSizeOfRegion(Memory::Region::HRAM), 0);
	memory_.emplace_back(Memory::GetSizeOfRegion(Memory::Region::Interrupts), 0);

	WriteByte(Memory::JOYP, 0xCF);
	WriteByte(Memory::TIMA, 0x00);
	WriteByte(Memory::TMA, 0x00);
	WriteByte(Memory::TAC, 0x00);
	WriteByte(0xFF10, 0x80); // NR10
	WriteByte(0xFF11, 0xBF); // NR11
	WriteByte(0xFF12, 0xF3); // NR12
	WriteByte(0xFF14, 0xBF); // NR14
	WriteByte(0xFF16, 0x3F); // NR21
	WriteByte(0xFF17, 0x00); // NR22
	WriteByte(0xFF19, 0xBF); // NR24
	WriteByte(0xFF1A, 0x7F); // NR30
	WriteByte(0xFF1B, 0xFF); // NR31
	WriteByte(0xFF1C, 0x9F); // NR32
	WriteByte(0xFF1E, 0xBF); // NR33
	WriteByte(0xFF20, 0xFF); // NR41
	WriteByte(0xFF21, 0x00); // NR42
	WriteByte(0xFF22, 0x00); // NR43
	WriteByte(0xFF23, 0xBF); // NR30
	WriteByte(0xFF24, 0x77); // NR50
	WriteByte(0xFF25, 0xF3); // NR51
	WriteByte(0xFF26, 0xF1); // NR52
	WriteByte(Memory::LCDC, 0x91);
	WriteByte(Memory::SCY, 0x00);
	WriteByte(Memory::SCX, 0x00);
	WriteByte(Memory::LYC, 0x00);
	WriteByte(Memory::BGP, 0xFC);
	WriteByte(Memory::OBP0, 0xFF);
	WriteByte(Memory::OBP1, 0xFF);
	WriteByte(Memory::WY, 0x00);
	WriteByte(Memory::WX, 0x00);
	WriteByte(Memory::IE, 0x00);
}

uint8_t MMU::ReadByte(Memory::Address address) const
{
	const auto region_and_relative_address = Memory::GetRegionAndRelativeAddress(address);

	if (is_oam_dma_active_ && (region_and_relative_address.first == Memory::Region::OAM)) return 0xFF;

	return memory_[static_cast<size_t>(region_and_relative_address.first)][region_and_relative_address.second];
}

void MMU::WriteByte(Memory::Address address, uint8_t value, bool notify)
{
	const auto region_and_relative_address = Memory::GetRegionAndRelativeAddress(address);

	if (is_oam_dma_active_ && (region_and_relative_address.first == Memory::Region::OAM)) return;

	switch (region_and_relative_address.first)
	{
	case Memory::Region::ROM_Bank0:
	case Memory::Region::ROM_OtherBanks:
		if (notify) NotifyMemoryWrite(region_and_relative_address.first, address, value);
		return;
	case Memory::Region::VRAM:
		//TODO: ignore writes during VRAM GPU state
		break;
	case Memory::Region::ERAM:
		if (!external_ram_enabled_) return;
		if ((address - Memory::external_ram_start_) > memory_[static_cast<size_t>(Memory::Region::ERAM)].size()) return;
		break;
	case Memory::Region::OAM:
		//TODO: ignore writes during OAM and VRAM GPU states
		break;
	default:
		break;
	}

	memory_[static_cast<size_t>(region_and_relative_address.first)][region_and_relative_address.second] = value;

	if (notify) NotifyMemoryWrite(region_and_relative_address.first, address, value);
}

void MMU::LoadRom(const std::string &rom_file_path)
{
	// Clear all state of previously loaded ROM
	rom_loaded_ = false;
	rom_banks.clear();
	external_ram_banks.clear();
	mbc_.reset();

	std::ifstream rom_read_stream{ rom_file_path, std::ios::binary | std::ios::ate };
	if (!rom_read_stream.is_open()) { throw std::runtime_error{ "ROM file could not be opened" }; }

	const auto file_size = static_cast<size_t>(rom_read_stream.tellg());
	rom_read_stream.seekg(0, std::ios::beg);

	//TODO: check byte 0x147 for number of ROM banks
	for (auto i = 0; i < file_size / Memory::GetSizeOfRegion(Memory::Region::ROM_Bank0); ++i)
	{
		rom_banks.emplace_back();
		rom_banks.back().resize(Memory::GetSizeOfRegion(Memory::Region::ROM_Bank0));
		rom_read_stream.read(reinterpret_cast<char*>(rom_banks.back().data()), rom_banks.back().size());
	}

	rom_read_stream.close();
	rom_loaded_ = true;

	// Map ROM banks 0 and 1
	memory_[static_cast<size_t>(Memory::Region::ROM_Bank0)].swap(rom_banks[0]);
	memory_[static_cast<size_t>(Memory::Region::ROM_OtherBanks)].swap(rom_banks[1]);
	loaded_rom_bank_ = 1;

	// Create the appropriate MBC
	switch (memory_[static_cast<size_t>(Memory::Region::ROM_Bank0)][0x147])
	{
	case 0:
		// No MBC
		break;
	case 1:
	case 2:
	case 3:
		mbc_ = std::make_unique<Mbc1>(*this);
		break;
	default:
		throw std::logic_error{ "Unsupported MBC:" + std::to_string(static_cast<int>(memory_[static_cast<size_t>(Memory::Region::ROM_Bank0)][0x147])) };
	}

	// Create the necessary number of external RAM banks
	if (mbc_) external_ram_banks = mbc_->GetExternalRamBanks(memory_[static_cast<size_t>(Memory::Region::ROM_Bank0)][0x149]);

	// Map the first external RAM bank
	if (!external_ram_banks.empty()) memory_[static_cast<size_t>(Memory::Region::ERAM)].swap(external_ram_banks[0]);
}

void MMU::LoadRomBank(size_t rom_bank_number)
{
	assert(rom_bank_number != 0);
	if (rom_bank_number >= rom_banks.size()) throw std::invalid_argument("Requested invalid ROM bank: " + std::to_string(rom_bank_number));

	if (rom_bank_number == loaded_rom_bank_) return;

	// Swap the currently loaded ROM bank back into its original slot
	memory_[static_cast<size_t>(Memory::Region::ROM_OtherBanks)].swap(rom_banks[loaded_rom_bank_]);

	// Then load the requested ROM bank into the main memory slot
	memory_[static_cast<size_t>(Memory::Region::ROM_OtherBanks)].swap(rom_banks[rom_bank_number]);

	loaded_rom_bank_ = rom_bank_number;
}

void MMU::LoadRamBank(size_t external_ram_bank_number)
{
	if (external_ram_bank_number >= external_ram_banks.size()) throw std::invalid_argument("Requested invalid external RAM bank: " + std::to_string(external_ram_bank_number));

	if (external_ram_bank_number == loaded_external_ram_bank_) return;

	// Swap the currently loaded external RAM bank back into its original slot
	memory_[static_cast<size_t>(Memory::Region::ERAM)].swap(external_ram_banks[loaded_external_ram_bank_]);

	// Then load the requested external RAM bank into the main memory slot
	memory_[static_cast<size_t>(Memory::Region::ERAM)].swap(external_ram_banks[external_ram_bank_number]);

	loaded_external_ram_bank_ = external_ram_bank_number;
}

#pragma region Watchpoints
bool MMU::IsReadWatchpointHit(Memory::Address address) const
{
	return read_watchpoints_.count(address) != 0;
}

bool MMU::IsWriteWatchpointHit(Memory::Address address) const
{
	return write_watchpoints_.count(address) != 0;
}
#pragma endregion

#pragma region GUI interaction
Memory::Map MMU::GetMemoryMap() const
{
	Memory::Map memory_map{};
	size_t offset{ 0 };

	for (int i = 0; i < memory_.size(); ++i)
	{
		memcpy(memory_map.data() + offset, memory_[i].data(), memory_[i].size());

		// memory_[i].size() cannot be used below, since in the MBC2 case the external RAM size will be 2kBytes, rather than the usual 8 kBytes
		offset += GetSizeOfRegion(static_cast<Memory::Region>(i));
	}
	
	return memory_map;
}

std::vector<Memory::Watchpoint> MMU::GetWatchpointList() const
{
	std::vector<Memory::Watchpoint> watchpoints;
	for (auto watchpoint_address : read_watchpoints_)
	{
		watchpoints.emplace_back(watchpoint_address, Memory::Watchpoint::Type::Read);
	}
	for (auto watchpoint_address : write_watchpoints_)
	{
		watchpoints.emplace_back(watchpoint_address, Memory::Watchpoint::Type::Write);
	}
	return watchpoints;
}

void MMU::AddWatchpoint(Memory::Watchpoint watchpoint)
{
	switch (watchpoint.type)
	{
	case Memory::Watchpoint::Type::Read:
		read_watchpoints_.emplace(watchpoint.address);
		break;
	case Memory::Watchpoint::Type::Write:
		write_watchpoints_.emplace(watchpoint.address);
		break;
	default:
		break;
	}
}

void MMU::RemoveWatchpoint(Memory::Watchpoint watchpoint)
{
	switch (watchpoint.type)
	{
	case Memory::Watchpoint::Type::Read:
		read_watchpoints_.erase(watchpoint.address);
		break;
	case Memory::Watchpoint::Type::Write:
		write_watchpoints_.erase(watchpoint.address);
		break;
	default:
		break;
	}
}
#pragma endregion

#pragma region Listener notification
void MMU::NotifyMemoryWrite(Memory::Region region, Memory::Address address, uint8_t value)
{
	for (auto& listener : listeners_[region])
	{
		listener(address, value);
	}
}
#pragma endregion
