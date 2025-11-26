import Constants from 'expo-constants';

export type VaralMode = 'AUTO' | 'FORCE_OPEN' | 'FORCE_CLOSE';
export type Command = 'AUTO' | 'OPEN' | 'CLOSE';

export interface Heartbeat {
  temp_c?: number | null;
  humidity?: number | null;
  rain?: boolean | null;
  mode?: VaralMode | null;
  uptime_ms?: number | null;
  received_at?: number | null;
}

const extra = (Constants.expoConfig?.extra ?? {}) as { apiBaseUrl?: string };
const envBaseUrl = typeof process !== 'undefined' ? process.env?.EXPO_PUBLIC_API_URL : undefined;
const API_BASE_URL = extra.apiBaseUrl ?? envBaseUrl ?? 'http://10.0.2.2:8000';

async function parseJson<T>(response: Response): Promise<T> {
  const text = await response.text();
  try {
    return JSON.parse(text) as T;
  } catch (error) {
    console.warn('Unable to parse JSON', error, text);
    throw new Error('Resposta inválida do servidor');
  }
}

export async function fetchHeartbeat(signal?: AbortSignal): Promise<Heartbeat> {
  const endpoint = `${API_BASE_URL.replace(/\/$/, '')}/heartbeat/`;
  const response = await fetch(endpoint, { signal });

  if (!response.ok) {
    throw new Error('Não foi possível carregar os dados do varal.');
  }

  return parseJson<Heartbeat>(response);
}

export async function sendCommand(command: Command): Promise<void> {
  const endpoint = `${API_BASE_URL.replace(/\/$/, '')}/cmd/`;
  const response = await fetch(endpoint, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ command }),
  });

  if (!response.ok) {
    const errorBody = await response.text().catch(() => '');
    if (errorBody) {
      try {
        const parsed = JSON.parse(errorBody);
        const detail = typeof parsed?.detail === 'string' ? parsed.detail : undefined;
        if (detail) {
          throw new Error(detail);
        }
      } catch (parseError) {
        if (!(parseError instanceof SyntaxError)) {
          throw parseError;
        }
      }
    }
    throw new Error(errorBody || 'Não foi possível enviar o comando.');
  }
}
