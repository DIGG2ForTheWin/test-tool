import os
import re
from pathlib import Path
from typing import Any, Dict, List, Optional

# Installed via requirements.txt in the Docker image
from dotenv import load_dotenv
import yaml


def load_config(config_path: str = "tests.yaml") -> Dict[str, Any]:
    file_path = Path(config_path)

    # Load .env (if present) from the same directory as the config file.
    # Variables that are already set (e.g. via docker --env-file) are not overwritten.
    load_dotenv(file_path.resolve().parent / ".env", override=False)
    if not file_path.exists():
        raise FileNotFoundError(f"Config file '{config_path}' was not found.")

    with open(file_path, "r", encoding="utf-8") as f:
        return _expand_env(yaml.safe_load(f))


_ENV_PATTERN = re.compile(r"\$\{([A-Za-z_][A-Za-z0-9_]*)\}")


def _expand_env(value: Any) -> Any:
    """Replace ${VAR} in string values with environment variables (e.g. passwords)."""
    if isinstance(value, dict):
        return {k: _expand_env(v) for k, v in value.items()}
    if isinstance(value, list):
        return [_expand_env(v) for v in value]
    if isinstance(value, str):
        def repl(m):
            name = m.group(1)
            if name not in os.environ:
                raise KeyError(f"Environment variable '{name}' is not set (referenced in the config file).")
            return os.environ[name]
        return _ENV_PATTERN.sub(repl, value)
    return value


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
            f"Test '{test_name}' is not available. Available tests: {', '.join(available)}"
        )

    return selected