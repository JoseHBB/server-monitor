from __future__ import annotations

import os
from dataclasses import asdict, dataclass
from typing import Any

import docker
import psutil
from flask import Flask, jsonify


CPU_TDP_WATTS = float(os.getenv("CPU_TDP_WATTS", "65"))
API_HOST = os.getenv("API_HOST", "0.0.0.0")
API_PORT = int(os.getenv("API_PORT", "8000"))

app = Flask(__name__)


@dataclass
class ContainerSummary:
    running: int
    inactive: int
    total: int
    projects: dict[str, dict[str, int]]


def get_system_metrics() -> dict[str, Any]:
    cpu_percent = psutil.cpu_percent(interval=1)
    disk = psutil.disk_usage("/")
    estimated_power_watts = CPU_TDP_WATTS * (cpu_percent / 100.0)
    estimated_energy_kwh_month = (estimated_power_watts * 24 * 30) / 1000.0

    return {
        "cpu": {
            "percent": round(cpu_percent, 2),
            "tdp_watts": CPU_TDP_WATTS,
        },
        "disk": {
            "path": "/",
            "free_bytes": disk.free,
            "free_gb": round(disk.free / (1024**3), 2),
            "used_percent": round(disk.percent, 2),
        },
        "energy_estimate": {
            "instant_power_watts": round(estimated_power_watts, 2),
            "projected_monthly_kwh": round(estimated_energy_kwh_month, 2),
            "logic": "TDP * (uso_cpu_percent / 100)",
        },
    }


def get_compose_container_summary() -> ContainerSummary:
    client = docker.from_env()
    containers = client.containers.list(
        all=True,
        filters={"label": "com.docker.compose.project"},
    )

    projects: dict[str, dict[str, int]] = {}
    running = 0
    inactive = 0

    for container in containers:
        project_name = container.labels.get("com.docker.compose.project", "unknown")
        project_stats = projects.setdefault(project_name, {"running": 0, "inactive": 0})

        if container.status == "running":
            running += 1
            project_stats["running"] += 1
        else:
            inactive += 1
            project_stats["inactive"] += 1

    return ContainerSummary(
        running=running,
        inactive=inactive,
        total=len(containers),
        projects=projects,
    )


@app.get("/metrics")
def metrics() -> Any:
    payload = get_system_metrics()

    try:
        compose_summary = get_compose_container_summary()
        payload["containers"] = asdict(compose_summary)
    except docker.errors.DockerException as exc:
        payload["containers"] = {
            "running": 0,
            "inactive": 0,
            "total": 0,
            "projects": {},
            "error": str(exc),
        }

    return jsonify(payload)


if __name__ == "__main__":
    app.run(host=API_HOST, port=API_PORT)
