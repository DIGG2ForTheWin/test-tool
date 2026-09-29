from pathlib import Path
from typing import Any, Dict, List, Optional
import yaml


def load_config(config_path: str = "tests.yaml") -> Dict[str, Any]:
    file_path = Path(config_path)
    if not file_path.exists():
        raise FileNotFoundError(f"Config-file '{config_path}' was not found.")

    with open(file_path, "r", encoding="utf-8") as f:
        return yaml.safe_load(f)


def get_test_cases(
    config: Dict[str, Any], test_name: Optional[str] = None
) -> List[Dict[str, Any]]:
    all_tests = config.get("tests", [])

    if not test_name or test_name.lower() == "all":
        return all_tests

    selected = [t for t in all_tests if t.get("name") == test_name]
    if not selected:
        available = [t.get("name") for t in all_tests]
        raise ValueError(
            f"Tests '{test_name}' not avalible. Avalible tests: {', '.join(available)}"
        )

    return selected