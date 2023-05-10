#include "duckdb/execution/index/art/prefix.hpp"

#include "duckdb/execution/index/art/art.hpp"
#include "duckdb/execution/index/art/art_key.hpp"
#include "duckdb/execution/index/art/node.hpp"
#include "duckdb/storage/meta_block_reader.hpp"
#include "duckdb/storage/meta_block_writer.hpp"

namespace duckdb {

void Prefix::New(ART &art, Node &node, const ARTKey &key, const uint32_t depth,
                 uint32_t count) {

	if (count == 0) {
		return;
	}
	idx_t copy_count = 0;

	while (count) {
		node.SetPtr(Node::GetAllocator(art, NType::PREFIX).New());
		node.type = (uint8_t)NType::PREFIX;
		auto &prefix = Prefix::Get(art, node);

		auto this_count = MinValue(uint32_t(Node::PREFIX_SIZE - 1), count);
		prefix.data[0] = (uint8_t)this_count;
		memcpy(prefix.data + 1, key.data + depth + copy_count, this_count);

		node = prefix.ptr;
		copy_count += this_count;
		count -= this_count;
	}
}

bool Prefix::FindMismatchPosition(const ART &art, Node &node, const ARTKey &key,
                                   const uint32_t depth, idx_t &compare_count) {

	D_ASSERT(node.DecodeARTNodeType() == NType::PREFIX);
	while (node.DecodeARTNodeType() == NType::PREFIX) {

		auto &prefix = Prefix::Get(art, node);
		for (idx_t i = 0; i < prefix.data[0]; i++) {
			// mismatch position
			if (prefix.data[i + 1] != key[depth + compare_count]) {
				return true;
			}
			compare_count++;
		}

		// no mismatch position in this prefix node, compare to next one
		node = prefix.ptr;
	}

	// prefix matches the key
	return false;
}

uint8_t Prefix::Split(ART &art, Node &node, Node &remaining_node, const idx_t compare_count) {

	D_ASSERT(node.DecodeARTNodeType() == NType::PREFIX);
	auto &prefix = Prefix::Get(art, node);

	// adjust the count of the prefix node
	uint8_t split_position = compare_count % Node::PREFIX_SIZE;
	auto split_byte = prefix.data[1 + split_position];

	// we might have to move all prefix bytes after split_position to a new
	// chain of prefix segments
	if (split_position + 1 < prefix.data[0] || prefix.ptr.DecodeARTNodeType() == NType::PREFIX) {

		idx_t copy_count = 0;
		reference<Node> current_node(remaining_node);

		while ()
		if (copy_count)

		Prefix::New(art, )
	}


	// if the new count is zero, then we free the prefix (chain) and don't change &node
	if (split_position == 0) {
		Node::Free(art, node);

	} else {
		// we adjust the count and &node to the subsequent node
		prefix.data[0] = split_position;
		node = prefix.ptr;
	}




}

void Prefix::Free(ART &art) {

	if (IsInlined()) {
		return Initialize();
	}

	// delete all prefix segments
	auto ptr = data.ptr;
	while (ptr.IsSet()) {
		auto next_ptr = PrefixSegment::Get(art, ptr).next;
		Node::Free(art, ptr);
		ptr = next_ptr;
	}

	Initialize();
}

void Prefix::InitializeMerge(ART &art, const idx_t buffer_count) {

	if (IsInlined()) {
		return;
	}

	reference<PrefixSegment> segment(PrefixSegment::Get(art, data.ptr));
	data.ptr.buffer_id += buffer_count;

	auto ptr = segment.get().next;
	while (ptr.IsSet()) {
		segment.get().next.buffer_id += buffer_count;
		segment = PrefixSegment::Get(art, ptr);
		ptr = segment.get().next;
	}
}

void Prefix::Append(ART &art, const Prefix &other) {

	// result fits into inlined data, i.e., both prefixes are also inlined
	if (count + other.count <= Node::PREFIX_INLINE_BYTES) {
		memcpy(data.inlined + count, other.data.inlined, other.count);
		count += other.count;
		return;
	}

	// this prefix is inlined, but will no longer be after appending the other prefix,
	// move the inlined bytes to the first prefix segment
	if (IsInlined()) {
		MoveInlinedToSegment(art);
	}

	// get the tail of the segments of this prefix
	reference<PrefixSegment> segment(PrefixSegment::Get(art, data.ptr).GetTail(art));

	// the other prefix is inlined
	if (other.IsInlined()) {
		for (idx_t i = 0; i < other.count; i++) {
			segment = segment.get().Append(art, count, other.data.inlined[i]);
		}
		return;
	}

	// iterate all segments of the other prefix and copy their data
	auto other_ptr = other.data.ptr;
	auto remaining = other.count;

	while (other_ptr.IsSet()) {
		auto &other_segment = PrefixSegment::Get(art, other_ptr);
		auto copy_count = MinValue(Node::PREFIX_SEGMENT_SIZE, remaining);

		// copy the data
		for (idx_t i = 0; i < copy_count; i++) {
			segment = segment.get().Append(art, count, other_segment.bytes[i]);
		}

		// adjust the loop variables
		other_ptr = other_segment.next;
		remaining -= copy_count;
	}
	D_ASSERT(remaining == 0);
}

void Prefix::Concatenate(ART &art, const uint8_t byte, const Prefix &other) {

	auto new_size = count + 1 + other.count;

	// overwrite into this prefix (both are inlined)
	if (new_size <= Node::PREFIX_INLINE_BYTES) {
		// move this prefix backwards
		memmove(data.inlined + other.count + 1, data.inlined, count);
		// copy byte
		data.inlined[other.count] = byte;
		// copy the other prefix into this prefix
		memcpy(data.inlined, other.data.inlined, other.count);
		count = new_size;
		return;
	}

	auto this_inlined = IsInlined();
	auto this_count = count;
	auto this_data = data;
	Initialize();

	// append the other prefix and possibly move the data to a segment
	Append(art, other);
	if (IsInlined()) {
		MoveInlinedToSegment(art);
	}

	// get the tail
	reference<PrefixSegment> segment(PrefixSegment::Get(art, data.ptr).GetTail(art));
	// append the byte
	segment = segment.get().Append(art, count, byte);

	if (this_inlined) {
		// append this prefix
		for (idx_t i = 0; i < this_count; i++) {
			segment = segment.get().Append(art, count, this_data.inlined[i]);
		}
		return;
	}

	// iterate all segments of this prefix, copy their data, and free them
	auto this_ptr = this_data.ptr;
	auto remaining = this_count;

	while (this_ptr.IsSet()) {
		auto &this_segment = PrefixSegment::Get(art, this_ptr);
		auto copy_count = MinValue(Node::PREFIX_SEGMENT_SIZE, remaining);

		// copy the data
		for (idx_t i = 0; i < copy_count; i++) {
			segment = segment.get().Append(art, count, this_segment.bytes[i]);
		}

		// adjust the loop variables
		Node::Free(art, this_ptr);
		this_ptr = this_segment.next;
		remaining -= copy_count;
	}
	D_ASSERT(remaining == 0);
}

uint8_t Prefix::Reduce(ART &art, const idx_t reduce_count) {

	auto new_count = count - reduce_count - 1;
	auto new_first_byte = GetByte(art, reduce_count);

	// prefix is now empty
	if (new_count == 0) {
		Free(art);
		return new_first_byte;
	}

	// was inlined, just move bytes
	if (IsInlined()) {
		memmove(data.inlined, data.inlined + reduce_count + 1, new_count);
		count = new_count;
		return new_first_byte;
	}

	count = 0;
	auto start = reduce_count + 1;
	auto offset = start % Node::PREFIX_SEGMENT_SIZE;
	auto remaining = new_count;

	// get the source segment, i.e., the segment that contains the byte at start
	reference<PrefixSegment> src_segment(PrefixSegment::Get(art, data.ptr));
	for (idx_t i = 0; i < start / Node::PREFIX_SEGMENT_SIZE; i++) {
		D_ASSERT(src_segment.get().next.IsSet());
		src_segment = PrefixSegment::Get(art, src_segment.get().next);
	}

	// iterate all segments starting at the source segment and shift their data
	reference<PrefixSegment> dst_segment(PrefixSegment::Get(art, data.ptr));
	while (true) {
		auto copy_count = MinValue(Node::PREFIX_SEGMENT_SIZE - offset, remaining);

		// copy the data
		for (idx_t i = offset; i < offset + copy_count; i++) {
			dst_segment = dst_segment.get().Append(art, count, src_segment.get().bytes[i]);
		}

		// adjust the loop variables
		offset = 0;
		remaining -= copy_count;
		if (remaining == 0) {
			break;
		}
		D_ASSERT(src_segment.get().next.IsSet());
		src_segment = PrefixSegment::Get(art, src_segment.get().next);
	}

	// possibly inline the data
	if (IsInlined()) {
		MoveSegmentToInlined(art);
	}

	return new_first_byte;
}

uint8_t Prefix::GetByte(const ART &art, const idx_t position) const {

	D_ASSERT(position < count);
	if (IsInlined()) {
		return data.inlined[position];
	}

	// get the correct segment
	reference<PrefixSegment> segment(PrefixSegment::Get(art, data.ptr));
	for (idx_t i = 0; i < position / Node::PREFIX_SEGMENT_SIZE; i++) {
		D_ASSERT(segment.get().next.IsSet());
		segment = PrefixSegment::Get(art, segment.get().next);
	}

	return segment.get().bytes[position % Node::PREFIX_SEGMENT_SIZE];
}

uint32_t Prefix::MismatchPosition(const ART &art, const Prefix &other) const {

	D_ASSERT(count <= other.count);

	// case 1: both prefixes are inlined
	if (IsInlined() && other.IsInlined()) {
		for (uint32_t i = 0; i < count; i++) {
			if (data.inlined[i] != other.data.inlined[i]) {
				return i;
			}
		}
		return count;
	}

	// case 2: only this prefix is inlined
	if (IsInlined()) {
		// we only need the first segment of the other prefix
		auto &segment = PrefixSegment::Get(art, other.data.ptr);
		for (uint32_t i = 0; i < count; i++) {
			if (data.inlined[i] != segment.bytes[i]) {
				return i;
			}
		}
		return count;
	}

	// case 3: both prefixes are not inlined
	auto ptr = data.ptr;
	auto other_ptr = other.data.ptr;

	// iterate segments and compare bytes
	uint32_t mismatch_position = 0;
	while (ptr.IsSet()) {
		D_ASSERT(other_ptr.IsSet());
		auto &segment = PrefixSegment::Get(art, ptr);
		auto &other_segment = PrefixSegment::Get(art, other_ptr);

		// compare bytes
		auto compare_count = MinValue(Node::PREFIX_SEGMENT_SIZE, count - mismatch_position);
		for (uint32_t i = 0; i < compare_count; i++) {
			if (segment.bytes[i] != other_segment.bytes[i]) {
				return mismatch_position;
			}
			mismatch_position++;
		}

		// adjust loop variables
		ptr = segment.next;
		other_ptr = other_segment.next;
	}
	return count;
}

void Prefix::Serialize(const ART &art, MetaBlockWriter &writer) const {

	writer.Write(count);

	// write inlined data
	if (IsInlined()) {
		writer.WriteData(data.inlined, count);
		return;
	}

	D_ASSERT(data.ptr.IsSet());
	auto ptr = data.ptr;
	auto remaining = count;

	// iterate all prefix segments and write their bytes
	while (ptr.IsSet()) {
		auto &segment = PrefixSegment::Get(art, ptr);
		auto copy_count = MinValue(Node::PREFIX_SEGMENT_SIZE, remaining);

		// write the bytes
		writer.WriteData(segment.bytes, copy_count);

		// adjust loop variables
		remaining -= copy_count;
		ptr = segment.next;
	}
	D_ASSERT(remaining == 0);
}

void Prefix::Deserialize(ART &art, MetaBlockReader &reader) {

	auto count_p = reader.Read<uint32_t>();

	// copy into inlined data
	if (count_p <= Node::PREFIX_INLINE_BYTES) {
		reader.ReadData(data.inlined, count_p);
		count = count_p;
		return;
	}

	// copy into segments
	count = 0;
	reference<PrefixSegment> segment(PrefixSegment::New(art, data.ptr));
	for (idx_t i = 0; i < count_p; i++) {
		segment = segment.get().Append(art, count, reader.Read<uint8_t>());
	}
	D_ASSERT(count_p == count);
}

void Prefix::Vacuum(ART &art) {

	if (IsInlined()) {
		return;
	}

	// first pointer has special treatment because we don't obtain it from a prefix segment
	auto &allocator = Node::GetAllocator(art, NType::PREFIX_SEGMENT);
	if (allocator.NeedsVacuum(data.ptr)) {
		data.ptr.SetPtr(allocator.VacuumPointer(data.ptr));
	}

	auto ptr = data.ptr;
	while (ptr.IsSet()) {
		auto &segment = PrefixSegment::Get(art, ptr);
		ptr = segment.next;
		if (ptr.IsSet() && allocator.NeedsVacuum(ptr)) {
			segment.next.SetPtr(allocator.VacuumPointer(ptr));
			ptr = segment.next;
		}
	}
}

PrefixSegment &Prefix::MoveInlinedToSegment(ART &art) {

	D_ASSERT(IsInlined());

	Node ptr;
	auto &segment = PrefixSegment::New(art, ptr);

	// move data
	D_ASSERT(Node::PREFIX_SEGMENT_SIZE >= Node::PREFIX_INLINE_BYTES);
	memcpy(segment.bytes, data.inlined, count);
	data.ptr = ptr;
	return segment;
}

void Prefix::MoveSegmentToInlined(ART &art) {

	D_ASSERT(IsInlined());
	D_ASSERT(data.ptr.IsSet());

	auto ptr = data.ptr;
	auto &segment = PrefixSegment::Get(art, data.ptr);

	memcpy(data.inlined, segment.bytes, count);
	Node::Free(art, ptr);
}

} // namespace duckdb
