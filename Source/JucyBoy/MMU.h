#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <list>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include "Memory.h"
#include "IMbc.h"

class MMU
{
public:
	MMU();
	~MMU();

	// Sets certain memory registers' initial state
	void Reset();

	uint8_t ReadByte(Memory::Address address) const;
	void WriteByte(Memory::Address address, uint8_t value, bool notify = true);
	bool IsReadWatchpointHit(Memory::Address address) const;
	bool IsWriteWatchpointHit(Memory::Address address) const;

	template <int BitNum>
	void SetBit(Memory::Address address, bool notify = true) { WriteByte(address, (1 << BitNum) | ReadByte(address), notify); }
	template <int BitNum>
	void ClearBit(Memory::Address address, bool notify = true) { WriteByte(address, ~(1 << BitNum) & ReadByte(address), notify); }
	template <int BitNum>
	bool IsBitSet(Memory::Address address) { return (ReadByte(address) & (1 << BitNum)) != 0; }

	void LoadRom(const std::string &rom_file_path);
	bool IsRomLoaded() const noexcept { return rom_loaded_; }

	// Functions called by IMbc
	void EnableExternalRam(bool enable) { external_ram_enabled_ = enable; }
	void LoadRomBank(size_t rom_bank_number);
	void LoadRamBank(size_t ram_bank_number);

	// Functions called by OamDma
	void OamDmaActive(bool is_active) { is_oam_dma_active_ = is_active; }

	// GUI interaction
	Memory::Map GetMemoryMap() const;
	std::vector<Memory::Watchpoint> GetWatchpointList() const;
	void AddWatchpoint(Memory::Watchpoint watchpoint);
	void RemoveWatchpoint(Memory::Watchpoint watchpoint);

	// AddListener returns a deregister function that can be called with no arguments
	template <typename T>
	std::function<void()> AddListener(T &listener, void(T::*func)(Memory::Address, uint8_t), Memory::Region region)
	{
		auto it = listeners_[region].emplace(listeners_[region].begin(), std::bind(func, std::ref(listener), std::placeholders::_1, std::placeholders::_2));
		return [=, this]() { listeners_[region].erase(it); };
	}

private:
	// Listener notification
	void NotifyMemoryWrite(Memory::Region region, Memory::Address address, uint8_t value);

private:
	std::vector<std::vector<uint8_t>> memory_;
	std::vector<std::vector<uint8_t>> rom_banks;
	std::vector<std::vector<uint8_t>> external_ram_banks;

	std::unique_ptr<IMbc> mbc_;

	bool rom_loaded_{ false };
	size_t loaded_rom_bank_{ 1 };
	size_t loaded_external_ram_bank_{ 0 };
	bool external_ram_enabled_{ false };

	bool is_oam_dma_active_{ false };

	// Watchpoints
	std::set<Memory::Address> read_watchpoints_;
	std::set<Memory::Address> write_watchpoints_;

	using Listener = std::function<void(Memory::Address address, uint8_t value)>;
	std::map<Memory::Region, std::list<Listener>> listeners_;
};
