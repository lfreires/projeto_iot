export function formatTemperature(value?: number | null): string {
  if (value === null || value === undefined) {
    return '—';
  }
  return `${value.toFixed(1)}`;
}

export function formatHumidity(value?: number | null): string {
  if (value === null || value === undefined) {
    return '—';
  }
  return `${value.toFixed(0)}`;
}

export function formatUptime(uptimeMs?: number | null): string {
  if (!uptimeMs) {
    return '—';
  }

  const totalSeconds = Math.floor(uptimeMs / 1000);
  const hours = Math.floor(totalSeconds / 3600);
  const minutes = Math.floor((totalSeconds % 3600) / 60);

  if (hours === 0 && minutes === 0) {
    return '< 1 min';
  }

  if (hours === 0) {
    return `${minutes} min`;
  }

  return `${hours}h ${minutes.toString().padStart(2, '0')}m`;
}

export function formatRelativeTime(timestampSeconds?: number | null): string {
  if (!timestampSeconds) {
    return 'Sem dados recentes';
  }

  const diffMs = Date.now() - timestampSeconds * 1000;
  const totalSeconds = Math.max(0, Math.floor(diffMs / 1000));

  if (totalSeconds < 10) {
    return 'agora mesmo';
  }
  if (totalSeconds < 60) {
    return `há ${totalSeconds}s`;
  }

  const minutes = Math.floor(totalSeconds / 60);
  if (minutes < 60) {
    return minutes === 1 ? 'há 1 min' : `há ${minutes} min`;
  }

  const hours = Math.floor(minutes / 60);
  if (hours < 24) {
    return hours === 1 ? 'há 1h' : `há ${hours}h`;
  }

  const days = Math.floor(hours / 24);
  return days === 1 ? 'há 1 dia' : `há ${days} dias`;
}
