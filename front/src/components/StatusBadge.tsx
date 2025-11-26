import React, { ComponentProps } from 'react';
import { StyleProp, StyleSheet, Text, View, ViewStyle } from 'react-native';
import { MaterialCommunityIcons } from '@expo/vector-icons';

import { colors, spacing, typography } from '../theme';

type IconName = ComponentProps<typeof MaterialCommunityIcons>['name'];

interface StatusBadgeProps {
  label: string;
  type: 'safe' | 'alert' | 'info';
  icon: IconName;
  style?: StyleProp<ViewStyle>;
}

const badgeColors = {
  safe: { background: '#DCFCE7', text: colors.success },
  alert: { background: '#FFE4E6', text: colors.danger },
  info: { background: '#E0F2FE', text: colors.rain },
};

export function StatusBadge({ label, type, icon, style }: StatusBadgeProps) {
  const palette = badgeColors[type];
  return (
    <View style={[styles.badge, style, { backgroundColor: palette.background }]}> 
      <MaterialCommunityIcons name={icon} size={16} color={palette.text} />
      <Text
        style={[styles.label, { color: palette.text }]}
        numberOfLines={1}
        ellipsizeMode="tail"
      >
        {label}
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  badge: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: spacing.xs,
    paddingHorizontal: spacing.sm,
    paddingVertical: spacing.xs,
    borderRadius: 999,
    flexShrink: 1,
    minWidth: 0,
  },
  label: {
    fontSize: typography.caption,
    fontWeight: '600',
    flexShrink: 1,
    minWidth: 0,
  },
});
