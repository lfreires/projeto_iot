import { useCallback, useEffect, useMemo, useRef, useState } from 'react';

import { fetchHeartbeat, Heartbeat } from '../services/api';

type ConnectionState = 'connecting' | 'online' | 'stale' | 'offline';

interface UseHeartbeatResult {
  data: Heartbeat | null;
  loading: boolean;
  error: string | null;
  refresh: () => Promise<void>;
  lastUpdatedLabel: string;
  connectionState: ConnectionState;
}

const REFRESH_MS = 10_000;
const STALE_HEARTBEAT_MS = 60_000;

export function useHeartbeat(autoRefresh = true): UseHeartbeatResult {
  const [data, setData] = useState<Heartbeat | null>(null);
  const [loading, setLoading] = useState<boolean>(true);
  const [error, setError] = useState<string | null>(null);
  const abortController = useRef<AbortController | null>(null);

  const refresh = useCallback(async () => {
    abortController.current?.abort();
    abortController.current = new AbortController();

    setLoading(true);
    setError(null);
    try {
      const heartbeat = await fetchHeartbeat(abortController.current.signal);
      setData(heartbeat);
    } catch (err) {
      if (err instanceof DOMException && err.name === 'AbortError') {
        return;
      }
      const message = err instanceof Error ? err.message : 'Erro inesperado';
      console.warn('Falha ao carregar heartbeat', message);
      setError('offline');
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    refresh();
    return () => abortController.current?.abort();
  }, [refresh]);

  useEffect(() => {
    if (!autoRefresh) return undefined;
    const interval = setInterval(refresh, REFRESH_MS);
    return () => clearInterval(interval);
  }, [autoRefresh, refresh]);

  const lastUpdatedLabel = useMemo(() => {
    if (!data?.received_at) {
      return 'Sem dados recentes';
    }
    const date = new Date(data.received_at * 1000);
    return `Atualizado às ${date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}`;
  }, [data?.received_at]);

  const connectionState: ConnectionState = useMemo(() => {
    const lastHeartbeatMs = data?.received_at ? data.received_at * 1000 : null;
    const isStale = Boolean(lastHeartbeatMs && Date.now() - lastHeartbeatMs > STALE_HEARTBEAT_MS);

    if (!lastHeartbeatMs && loading && !error) {
      return 'connecting';
    }

    if (error && !lastHeartbeatMs) {
      return 'offline';
    }

    if (isStale) {
      return error ? 'offline' : 'stale';
    }

    if (lastHeartbeatMs) {
      return error ? 'offline' : 'online';
    }

    return error ? 'offline' : 'connecting';
  }, [data?.received_at, error, loading]);

  return {
    data,
    loading,
    error,
    refresh,
    lastUpdatedLabel,
    connectionState,
  };
}
