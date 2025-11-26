import React, { ComponentProps, useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { ActivityIndicator, Animated, Pressable, RefreshControl, ScrollView, StyleSheet, Text, View } from 'react-native';
import { SafeAreaView } from 'react-native-safe-area-context';
import { MaterialCommunityIcons } from '@expo/vector-icons';

import { MetricCard } from '../components/MetricCard';
import { StatusBadge } from '../components/StatusBadge';
import { InfoRow } from '../components/InfoRow';
import { useHeartbeat } from '../hooks/useHeartbeat';
import { Command, VaralMode, sendCommand } from '../services/api';
import { colors, spacing, typography } from '../theme';
import { formatHumidity, formatRelativeTime, formatTemperature, formatUptime } from '../utils/format';

type IconName = ComponentProps<typeof MaterialCommunityIcons>['name'];

const COMMAND_TO_MODE: Record<Command, VaralMode> = {
  AUTO: 'AUTO',
  OPEN: 'FORCE_OPEN',
  CLOSE: 'FORCE_CLOSE',
};

const CONTROL_COMMANDS: Array<{
  command: Command;
  label: string;
  description: string;
  icon: IconName;
  matchMode: VaralMode;
}> = [
  {
    command: 'AUTO',
    label: 'Modo automático',
    description: 'Sensores controlam o varal automaticamente.',
    icon: 'autorenew',
    matchMode: 'AUTO',
  },
  {
    command: 'OPEN',
    label: 'Forçar abertura',
    description: 'Mantém o varal aberto até novo comando.',
    icon: 'arrow-up-bold-outline',
    matchMode: 'FORCE_OPEN',
  },
  {
    command: 'CLOSE',
    label: 'Forçar fechamento',
    description: 'Mantém o varal fechado até novo comando.',
    icon: 'arrow-down-bold-outline',
    matchMode: 'FORCE_CLOSE',
  },
];

export default function DashboardScreen() {
  const { data, loading, refresh, lastUpdatedLabel, connectionState } = useHeartbeat(true);
  const [commandLoading, setCommandLoading] = useState<Command | null>(null);
  const [optimisticMode, setOptimisticMode] = useState<VaralMode | null>(null);
  const [manualRefreshing, setManualRefreshing] = useState(false);
  const [pendingModeCommandAt, setPendingModeCommandAt] = useState<number | null>(null);
  const [awaitingHeartbeatAck, setAwaitingHeartbeatAck] = useState(false);
  const [feedback, setFeedback] = useState<{ type: 'success' | 'error'; message: string } | null>(null);

  const contentOpacity = useRef(new Animated.Value(0)).current;
  const feedbackOpacity = useRef(new Animated.Value(0)).current;
  const feedbackTimerRef = useRef<NodeJS.Timeout | null>(null);

  useEffect(() => {
    Animated.timing(contentOpacity, {
      toValue: data ? (loading ? 0.85 : 1) : 0.65,
      duration: 350,
      useNativeDriver: true,
    }).start();
  }, [contentOpacity, data, loading]);

  useEffect(() => {
    if (!feedback) {
      return undefined;
    }

    feedbackOpacity.setValue(0);
    Animated.timing(feedbackOpacity, {
      toValue: 1,
      duration: 200,
      useNativeDriver: true,
    }).start();

    if (feedbackTimerRef.current) {
      clearTimeout(feedbackTimerRef.current);
    }
    feedbackTimerRef.current = setTimeout(() => {
      feedbackTimerRef.current = null;
      Animated.timing(feedbackOpacity, {
        toValue: 0,
        duration: 250,
        useNativeDriver: true,
      }).start(({ finished }) => {
        if (finished) {
          setFeedback(null);
        }
      });
    }, 3500);

    return () => {
      if (feedbackTimerRef.current) {
        clearTimeout(feedbackTimerRef.current);
        feedbackTimerRef.current = null;
      }
    };
  }, [feedback, feedbackOpacity]);

  const rainStatus = data?.rain ? 'Chuva detectada' : 'Tempo seco';
  const rainBadgeType = data?.rain ? 'alert' : 'safe';
  const uptime = formatUptime(data?.uptime_ms);
  const lastMessageRelative = useMemo(() => (data?.received_at ? formatRelativeTime(data?.received_at) : null), [data?.received_at]);

  const lastHeartbeatMs = data?.received_at ? data.received_at * 1000 : null;
  const hasHeartbeatData = Boolean(lastHeartbeatMs);
  const usingHistoricData = connectionState === 'offline' || connectionState === 'stale' || !hasHeartbeatData;

  useEffect(() => {
    if (!pendingModeCommandAt) {
      return;
    }
    if (!lastHeartbeatMs || lastHeartbeatMs < pendingModeCommandAt) {
      return;
    }

    setPendingModeCommandAt(null);
    setOptimisticMode(null);
    setAwaitingHeartbeatAck(false);
  }, [lastHeartbeatMs, pendingModeCommandAt]);

  const connectionMeta = useMemo(() => {
    switch (connectionState) {
      case 'online':
        return {
          label: 'Online',
          icon: 'access-point-check' as IconName,
          color: colors.success,
          description: lastMessageRelative ? `Última mensagem ${lastMessageRelative}` : 'Fluxo em tempo real',
        };
      case 'stale':
        return {
          label: 'Sem dados novos',
          icon: 'clock-alert' as IconName,
          color: colors.warning,
          description: lastMessageRelative ? `Último pulso ${lastMessageRelative}` : 'Aguardando atualização',
        };
      case 'offline':
        return {
          label: 'Offline',
          icon: 'wifi-off' as IconName,
          color: colors.danger,
          description: lastMessageRelative ? `Última mensagem ${lastMessageRelative}` : 'Tentando restabelecer conexão',
        };
      default:
        return {
          label: 'Conectando...',
          icon: 'wifi-sync' as IconName,
          color: colors.rain,
          description: 'Tentando conectar ao backend',
        };
    }
  }, [connectionState, lastMessageRelative]);

  const isOffline = connectionState === 'offline';
  const isStale = connectionState === 'stale';
  const lastUpdatedMessage = lastUpdatedLabel.includes('Atualizado')
    ? lastUpdatedLabel
    : `Atualizado: ${lastUpdatedLabel}`;
  const lastUpdatedBadge = {
    label: lastUpdatedMessage,
    type: (isOffline || isStale ? 'alert' : 'info') as 'alert' | 'info',
    icon: (isOffline ? 'wifi-off' : isStale ? 'clock-alert' : 'clock-outline') as IconName,
  };
  const shouldShowOptimisticMode = Boolean(
    optimisticMode && pendingModeCommandAt !== null && (!lastHeartbeatMs || lastHeartbeatMs < pendingModeCommandAt),
  );

  const currentMode = shouldShowOptimisticMode ? optimisticMode : data?.mode ?? null;

  const modeLabel = useMemo(() => {
    const mode = currentMode;
    switch (mode) {
      case 'AUTO':
        return 'Automático';
      case 'FORCE_OPEN':
        return 'Aberto manualmente';
      case 'FORCE_CLOSE':
        return 'Fechado manualmente';
      default:
        return '—';
    }
  }, [currentMode]);

  const handleSendCommand = useCallback(async (command: Command) => {
    if (commandLoading) {
      return;
    }
    if (feedbackTimerRef.current) {
      clearTimeout(feedbackTimerRef.current);
      feedbackTimerRef.current = null;
    }
    setFeedback(null);
    feedbackOpacity.stopAnimation();
    feedbackOpacity.setValue(0);
    setCommandLoading(command);
    const targetMode = COMMAND_TO_MODE[command];
    setOptimisticMode(targetMode);
    setPendingModeCommandAt(Date.now());
    setAwaitingHeartbeatAck(true);
    try {
      await sendCommand(command);
      setFeedback({ type: 'success', message: 'Comando enviado com sucesso' });
      await refresh();
    } catch (err) {
      const message = err instanceof Error ? err.message : 'Erro ao enviar comando';
      setFeedback({ type: 'error', message });
      setOptimisticMode(null);
      setPendingModeCommandAt(null);
      setAwaitingHeartbeatAck(false);
    } finally {
      setCommandLoading(null);
    }
  }, [commandLoading, refresh, feedbackOpacity]);

  const handleManualRefresh = useCallback(async () => {
    if (manualRefreshing) {
      return;
    }
    setManualRefreshing(true);
    try {
      await refresh();
    } finally {
      setManualRefreshing(false);
    }
  }, [manualRefreshing, refresh]);

  const hasFeedback = Boolean(feedback);
  const feedbackPalette = feedback?.type === 'error'
    ? { backgroundColor: '#FEE4E2', borderColor: '#FDA29B', color: colors.danger, icon: 'alert-circle' as IconName }
    : { backgroundColor: '#DCFCE7', borderColor: '#86EFAC', color: colors.success, icon: 'check-circle' as IconName };

  return (
    <SafeAreaView style={styles.safeArea} edges={['top']}>
      <ScrollView
        style={styles.container}
        contentContainerStyle={styles.content}
        refreshControl={
          <RefreshControl refreshing={manualRefreshing} onRefresh={handleManualRefresh} tintColor={colors.primary} />
        }
      >
        <View style={styles.header}>
          <View style={styles.headerTitleRow}>
            <Text style={styles.title}>Varal inteligente</Text>
            <StatusBadge
              label={lastUpdatedBadge.label}
              type={lastUpdatedBadge.type}
              icon={lastUpdatedBadge.icon}
              style={styles.inlineBadge}
            />
          </View>
          <View style={styles.headerBadges}>
            <StatusBadge
              label={rainStatus}
              type={rainBadgeType}
              icon={data?.rain ? 'weather-pouring' : 'weather-sunny'}
            />
            <StatusBadge
              label={connectionMeta.label}
              type={connectionState === 'offline' ? 'alert' : connectionState === 'online' ? 'safe' : 'info'}
              icon={connectionMeta.icon}
            />
          </View>
        </View>
        <Animated.View style={[styles.metricsRow, { opacity: contentOpacity }]}> 
          <MetricCard
            label="Temperatura"
            value={formatTemperature(data?.temp_c)}
            unit="ºC"
            icon="thermometer"
          />
          <MetricCard
            label="Umidade"
            value={formatHumidity(data?.humidity)}
            unit="%"
            icon="water-percent"
          />
        </Animated.View>

        <View style={styles.section}>
          <Text style={styles.sectionTitle}>{usingHistoricData ? 'Últimos detalhes lidos' : 'Detalhes'}</Text>
          <View style={styles.card}>
            <InfoRow label={usingHistoricData ? 'Último uptime registrado' : 'Uptime'} value={uptime} />
            <InfoRow label={usingHistoricData ? 'Último status de chuva' : 'Chuva'} value={data?.rain ? 'Sim' : 'Não'} />
            <InfoRow label={usingHistoricData ? 'Último modo' : 'Modo atual'} value={modeLabel} />
            <InfoRow label={usingHistoricData ? 'Última temperatura lida' : 'Temperatura'} value={`${formatTemperature(data?.temp_c)} ºC`} />
            <InfoRow label={usingHistoricData ? 'Última umidade lida' : 'Umidade'} value={`${formatHumidity(data?.humidity)} %`} />
          </View>
        </View>

        {connectionState !== 'offline' && connectionState !== 'stale' ? (
          <View style={styles.section}>
            <Text style={styles.sectionTitle}>Controle rápido</Text>

            {hasFeedback ? (
              <Animated.View
                style={[
                  styles.feedbackBanner,
                  {
                    backgroundColor: feedbackPalette.backgroundColor,
                    borderColor: feedbackPalette.borderColor,
                    opacity: feedbackOpacity,
                  },
                ]}
              >
                <MaterialCommunityIcons name={feedbackPalette.icon} size={18} color={feedbackPalette.color} />
                <Text style={[styles.feedbackText, { color: feedbackPalette.color }]}>{feedback?.message}</Text>
              </Animated.View>
            ) : null}

            <View style={styles.controlsWrapper}>
              {CONTROL_COMMANDS.map(({ command, label, description, icon, matchMode }) => {
                const isCurrentMode = currentMode === matchMode;
                const isSending = commandLoading === command;
                const disabled = awaitingHeartbeatAck || Boolean(commandLoading) || isCurrentMode;
                return (
                  <Pressable
                    key={command}
                    onPress={() => handleSendCommand(command)}
                    disabled={disabled}
                    style={({ pressed }) => [
                      styles.controlButton,
                      isCurrentMode && styles.controlButtonActive,
                      disabled && styles.controlButtonDisabled,
                      pressed && !disabled ? styles.controlButtonPressed : null,
                    ]}
                  >
                    <View style={styles.controlButtonHeader}>
                      <View style={{ flex: 1 }}>
                        <Text style={styles.controlButtonLabel}>{label}</Text>
                        <Text style={styles.controlButtonDescription}>{description}</Text>
                      </View>
                      {isSending ? (
                        <ActivityIndicator size="small" color={colors.primary} />
                      ) : (
                        <MaterialCommunityIcons name={icon} size={20} color={colors.primary} />
                      )}
                    </View>
                    {isCurrentMode ? <Text style={styles.controlStatusPill}>Ativo</Text> : null}
                  </Pressable>
                );
              })}
            </View>
          </View>
        ) : null}
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  safeArea: {
    flex: 1,
    backgroundColor: colors.background,
  },
  container: {
    flex: 1,
    backgroundColor: colors.background,
  },
  content: {
    padding: spacing.lg,
    paddingBottom: spacing.xl,
  },
  header: {
    marginBottom: spacing.md,
  },
  headerTitleRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: spacing.sm,
    flexWrap: 'wrap',
  },
  title: {
    fontSize: typography.title,
    fontWeight: '700',
    color: colors.text,
  },
  subtitle: {
    fontSize: typography.body,
    color: colors.textMuted,
    marginTop: 4,
  },
  subtitleOffline: {
    color: colors.danger,
    fontWeight: '600',
  },
  headerBadges: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    alignItems: 'center',
    gap: spacing.xs,
    marginTop: spacing.sm,
  },
  inlineBadge: {
    marginLeft: 'auto',
  },
  metricsRow: {
    flexDirection: 'row',
    gap: spacing.md,
  },
  section: {
    marginTop: spacing.lg,
  },
  sectionTitle: {
    fontSize: typography.subtitle,
    fontWeight: '600',
    marginBottom: spacing.sm,
    color: colors.text,
  },
  card: {
    backgroundColor: colors.card,
    borderRadius: 16,
    paddingHorizontal: spacing.md,
    paddingVertical: spacing.sm,
    gap: spacing.xs,
    shadowColor: '#0000000F',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.08,
    shadowRadius: 6,
    elevation: 2,
  },
  controlsWrapper: {
    gap: spacing.md,
  },
  controlButton: {
    backgroundColor: colors.card,
    borderRadius: 18,
    padding: spacing.md,
    borderWidth: 1,
    borderColor: colors.border,
  },
  controlButtonPressed: {
    transform: [{ scale: 0.98 }],
  },
  controlButtonDisabled: {
    opacity: 0.6,
  },
  controlButtonActive: {
    borderColor: colors.primary,
    backgroundColor: `${colors.primary}0D`,
  },
  controlButtonHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: spacing.md,
  },
  controlButtonLabel: {
    fontSize: typography.subtitle,
    fontWeight: '600',
    color: colors.text,
  },
  controlButtonDescription: {
    fontSize: typography.caption,
    color: colors.textMuted,
    marginTop: spacing.xs,
  },
  controlStatusPill: {
    alignSelf: 'flex-start',
    marginTop: spacing.sm,
    paddingHorizontal: spacing.sm,
    paddingVertical: spacing.xs,
    borderRadius: 999,
    backgroundColor: '#DCFCE7',
    color: colors.success,
    fontSize: typography.caption,
    fontWeight: '600',
  },
  feedbackBanner: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: spacing.sm,
    padding: spacing.sm,
    borderWidth: 1,
    borderRadius: 12,
    marginBottom: spacing.md,
  },
  feedbackText: {
    fontSize: typography.body,
    fontWeight: '600',
  },
});
