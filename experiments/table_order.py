#!/usr/bin/env python3

from __future__ import annotations

from typing import Iterable, List, Sequence, Tuple, TypeVar

ISA_ORDER = ["ARM", "RISC-V", "LoongArch"]

MICROARCH_ORDER = [
    "Cortex-A520",
    "Cortex-A72",
    "Cortex-A73",
    "Cortex-A76",
    "Cortex-A78",
    "Cortex-A720",
    "Cortex-A725",
    "Cortex-X4",
    "Neoverse-N1",
    "Oryon",
    "Kunpeng Pro",
    "Icestorm",
    "Firestorm",
    "C906",
    "C908",
    "C910",
    "X60",
    "3A5000-HV",
]

ISA_BY_MICROARCH = {
    "Cortex-A53": "ARM",
    "Cortex-A55": "ARM",
    "Cortex-A520": "ARM",
    "Cortex-A72": "ARM",
    "Cortex-A73": "ARM",
    "Cortex-A76": "ARM",
    "Cortex-A78": "ARM",
    "Cortex-A720": "ARM",
    "Cortex-A725": "ARM",
    "Cortex-X4": "ARM",
    "Carmel": "ARM",
    "Neoverse-N1": "ARM",
    "Oryon": "ARM",
    "Kunpeng Pro": "ARM",
    "Icestorm": "ARM",
    "Firestorm": "ARM",
    "Blizzard": "ARM",
    "Avalanche": "ARM",
    "C906": "RISC-V",
    "C908": "RISC-V",
    "C910": "RISC-V",
    "X60": "RISC-V",
    "U54": "RISC-V",
    "U74": "RISC-V",
    "P550": "RISC-V",
    "BOOM": "RISC-V",
    "3A5000-HV": "LoongArch",
}

_MICROARCH_RANK = {name: idx for idx, name in enumerate(MICROARCH_ORDER)}
_ISA_RANK = {isa: idx for idx, isa in enumerate(ISA_ORDER)}

T = TypeVar("T")


def microarch_isa(name: str) -> str | None:
    return ISA_BY_MICROARCH.get(name)


def sort_rows(rows: Iterable[T], key) -> List[T]:
    def sort_key(row: T) -> Tuple[int, int, int, str]:
        name = key(row)
        isa = microarch_isa(name)
        isa_rank = _ISA_RANK.get(isa, len(_ISA_RANK))
        microarch_rank = _MICROARCH_RANK.get(name)

        if microarch_rank is not None:
            return (isa_rank, 0, microarch_rank, name)
        if isa in _ISA_RANK:
            return (isa_rank, 1, len(_MICROARCH_RANK), name)
        return (isa_rank, 2, len(_MICROARCH_RANK), name)

    return sorted(rows, key=sort_key)


def iter_grouped_rows(rows: Sequence[T], key) -> List[Tuple[str | None, List[T]]]:
    grouped: List[Tuple[str | None, List[T]]] = []
    current_isa: str | None = None
    current_rows: List[T] = []

    for row in sort_rows(rows, key=key):
        isa = microarch_isa(key(row))
        if current_rows and isa != current_isa:
            grouped.append((current_isa, current_rows))
            current_rows = []
        current_isa = isa
        current_rows.append(row)

    if current_rows:
        grouped.append((current_isa, current_rows))

    return grouped
