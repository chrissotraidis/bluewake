#!/usr/bin/env python3
"""Public synthetic checks for the retail JKRExpHeap allocBack contract."""

U32_MASK = 0xFFFFFFFF
BLOCK_HEADER_SIZE = 0x10


def u32(value):
    return value & U32_MASK


def alloc_back(block_address, block_size, request_size, group_id=0xFF):
    """Model CMemBlock::allocBack with GameCube 32-bit pointer arithmetic."""
    block_address = u32(block_address)
    block_size = u32(block_size)
    request_size = u32(request_size)
    if block_size < request_size + BLOCK_HEADER_SIZE:
        return None, {"size": block_size}

    new_block = u32(block_address + block_size - request_size)
    return u32(new_block + BLOCK_HEADER_SIZE), {
        "size": u32(block_size - request_size - BLOCK_HEADER_SIZE),
        "new_block": new_block,
        "new_group": group_id,
        "new_flags": 0x80,
    }


def append_used_list(head, tail, new_block):
    """Model appendUsedList's pointer links and tail publication."""
    if tail is not None:
        tail["next"] = new_block
        new_block["prev"] = tail
    else:
        new_block["prev"] = None
    new_block["next"] = None
    new_block["magic"] = 0x484D  # 'HM'
    return new_block if head is None else head, new_block


def test_retail_alloc_back_preserves_32_bit_semantics_for_observed_range():
    result, state = alloc_back(0x80AD2140, 0xBDE4, 0x20)
    assert state["new_block"] == 0x80ADDF04
    assert result == 0x80ADDF14
    assert state["size"] == 0xBDB4


def test_alloc_back_wraps_like_guest_u32_arithmetic():
    result, state = alloc_back(0xFFFFFFF0, 0x40, 0x20)
    assert state["new_block"] == 0x10
    assert result == 0x20


def test_append_used_list_publishes_tail_without_touching_free_links():
    head = {"next": None}
    old_tail = {"next": None}
    new_block = {}
    result_head, result_tail = append_used_list(head, old_tail, new_block)
    assert result_head is head
    assert result_tail is new_block
    assert old_tail["next"] is new_block
    assert new_block["prev"] is old_tail
    assert new_block["next"] is None
    assert new_block["magic"] == 0x484D


def test_authentic_corrupt_state_has_no_single_alloc_back_operand_solution():
    """The observed size and new-block writes cannot share one request operand."""
    old_size = 0xBDE4
    corrupt_size = 0x9B6A12D3
    free_block = 0x80AD2140
    observed_new_block = 0x1C17337B
    implied_request = u32(old_size - BLOCK_HEADER_SIZE - corrupt_size)
    predicted_new_block = u32(free_block + old_size - implied_request)
    assert implied_request == 0x6496AB01
    assert predicted_new_block == 0x1C173423
    assert predicted_new_block != observed_new_block
    assert u32(predicted_new_block - observed_new_block) == 0xA8


if __name__ == "__main__":
    test_retail_alloc_back_preserves_32_bit_semantics_for_observed_range()
    test_alloc_back_wraps_like_guest_u32_arithmetic()
    test_append_used_list_publishes_tail_without_touching_free_links()
    test_authentic_corrupt_state_has_no_single_alloc_back_operand_solution()
    print("All synthetic allocBack tests passed.")
