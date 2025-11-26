import React, { ComponentProps, useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { ActivityIndicator, Animated, Pressable, RefreshControl, ScrollView, StyleSheet, Text, View } from 'react-native';
import { SafeAreaView } from 'react-native-safe-area-context';
import { MaterialCommunityIcons } from '@expo/vector-icons';

import { MetricCard } from '../components/MetricCard';
import { StatusBadge } from '../components/StatusBadge';
import { InfoRow } from '../components/InfoRow';
import { useHeartbeat } from '../hooks/useHeartbeat';
import { Command, sendCommand } from '../services/api';
import { colors, spacing, typography } from '../theme';
import { formatHumidity, formatRelativeTime, formatTemperature, formatUptime } from '../utils/format';

type IconName = ComponentProps<typeof MaterialCommunityIcons>['name'];

const CONTROL_COMMANDS: Array<{
  command: Command;
  label: string;
  description: string;
  icon: IconName;
  matchMode: 'AUTO' | 'FORCE_OPEN' | 'FORCE_CLOSE';
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
  const [commandFeedback, setCommandFeedback] = useState<string | null>(null);
  const [commandError, setCommandError] = useState<string | null>(null);

  const contentOpacity = useRef(new Animated.Value(0)).current;
  const feedbackOpacity = useRef(new Animated.Value(0)).current;

  useEffect(() => {
    Animated.timing(contentOpacity, {
      toValue: data ? 1 : 0.65,
      duration: 350,
      useNativeDriver: true,
    }).start();
  }, [contentOpacity, data]);

  useEffect(() => {
    Animated.timing(feedbackOpacity, {
      toValue: commandFeedback || commandError ? 1 : 0,
      duration: 200,
      useNativeDriver: true,
    }).start();
  }, [commandError, commandFeedback, feedbackOpacity]);

  useEffect(() => {
    if (!commandFeedback && !commandError) {
      return undefined;
    }
    const timeout = setTimeout(() => {
      setCommandFeedback(null);
      setCommandError(null);
    }, 4000);
    return () => clearTimeout(timeout);
  }, [commandError, commandFeedback]);

  const rainStatus = data?.rain ? 'Chuva detectada' : 'Tempo seco';
  const rainBadgeType = data?.rain ? 'alert' : 'safe';
  const uptime = formatUptime(data?.uptime_ms);
  const lastMessageRelative = useMemo(() => (data?.received_at ? formatRelativeTime(data.received_at) : null), [data?.received_at]);
  const refreshing = loading && Boolean(data);

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

  const currentMode = data?.mode ?? null;

  const modeLabel = useMemo(() => {
    switch (data?.mode) {
      case 'AUTO':
        return 'Automático';
      case 'FORCE_OPEN':
        return 'Aberto manualmente';
      case 'FORCE_CLOSE':
        return 'Fechado manualmente';
      default:
        return '—';
    }
  }, [data?.mode]);

  const handleSendCommand = useCallback(async (command: Command) => {
    if (commandLoading) {
      return;
    }
    setCommandError(null);
    setCommandFeedback(null);
    setCommandLoading(command);
    try {
      await sendCommand(command);
      setCommandFeedback('Comando enviado com sucesso');
      await refresh();
    } catch (err) {
      const message = err instanceof Error ? err.message : 'Erro ao enviar comando';
      setCommandError(message);
    } finally {
      setCommandLoading(null);
    }
  }, [commandLoading, refresh]);

  const hasFeedback = Boolean(commandFeedback || commandError);
  const feedbackPalette = commandError
    ? { backgroundColor: '#FEE4E2', borderColor: '#FDA29B', color: colors.danger, icon: 'alert-circle' as IconName }
    : { backgroundColor: '#DCFCE7', borderColor: '#86EFAC', color: colors.success, icon: 'check-circle' as IconName };

  return (
    <SafeAreaView style={styles.safeArea} edges={['top']}>
      <ScrollView
        style={styles.container}
        contentContainerStyle={styles.content}
        refreshControl={<RefreshControl refreshing={refreshing} onRefresh={refresh} tintColor={colors.primary} />}
      >
        <View style={styles.header}>
          <Text style={styles.title}>Varal inteligente</Text>
          <Text style={styles.subtitle}>{lastUpdatedLabel}</Text>
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
            trend="stable"
          />
          <MetricCard
            label="Umidade"
            value={formatHumidity(data?.humidity)}
            unit="%"
            icon="water-percent"
            trend="stable"
          />
        </Animated.View>

        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Detalhes</Text>
          <View style={styles.card}>
            <InfoRow label="Uptime" value={uptime} />
            <InfoRow label="Chuva" value={data?.rain ? 'Sim' : 'Não'} />
            <InfoRow label="Modo atual" value={modeLabel} />
            <InfoRow label="Temperatura" value={`${formatTemperature(data?.temp_c)} ºC`} />
            <InfoRow label="Umidade" value={`${formatHumidity(data?.humidity)} %`} />
          </View>
        </View>

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
              <Text style={[styles.feedbackText, { color: feedbackPalette.color }]}>{commandError ?? commandFeedback}</Text>
            </Animated.View>
          ) : null}

          <View style={styles.controlsWrapper}>
            {CONTROL_COMMANDS.map(({ command, label, description, icon, matchMode }) => {
              const isCurrentMode = currentMode === matchMode;
              const isSending = commandLoading === command;
              const disabled = Boolean(commandLoading) || isCurrentMode;
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
  headerBadges: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    alignItems: 'center',
    gap: spacing.xs,
    marginTop: spacing.sm,
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
