CREATE TABLE IF NOT EXISTS `dc_aoeloot_preferences` (
  `player_guid` int unsigned NOT NULL,
  `aoe_enabled` tinyint(1) NOT NULL DEFAULT '1',
  `min_quality` tinyint unsigned NOT NULL DEFAULT '0' COMMENT '0=Poor, 1=Common, 2=Uncommon, 3=Rare, 4=Epic, 5=Legendary',
  `auto_skin` tinyint(1) NOT NULL DEFAULT '1',
  `smart_loot` tinyint(1) NOT NULL DEFAULT '1',
  `auto_vendor_poor` tinyint(1) NOT NULL DEFAULT '0' COMMENT 'Auto-vendor poor quality items',
  `ignored_items` text COMMENT 'Comma-separated list of item IDs to ignore',
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `show_messages` tinyint(1) NOT NULL DEFAULT '1' COMMENT 'Whether to show AoE loot info/debug messages (1=show, 0=hide)',
  PRIMARY KEY (`player_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci COMMENT='DarkChaos AoE Loot - Player Preferences';

CREATE TABLE IF NOT EXISTS `dc_aoeloot_accumulated` (
  `player_guid` int unsigned NOT NULL,
  `accumulated_gold` bigint unsigned NOT NULL DEFAULT '0' COMMENT 'Total gold looted via AoE',
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`player_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci COMMENT='DarkChaos AoE Loot - Accumulated Gold';


CREATE TABLE IF NOT EXISTS `dc_aoeloot_detailed_stats` (
  `player_guid` int unsigned NOT NULL,
  `total_items` int unsigned NOT NULL DEFAULT '0',
  `total_gold` bigint unsigned NOT NULL DEFAULT '0' COMMENT 'In copper',
  `poor_vendored` int unsigned NOT NULL DEFAULT '0',
  `vendor_gold` bigint unsigned NOT NULL DEFAULT '0' COMMENT 'Gold from auto-vendoring',
  `skinned` int unsigned NOT NULL DEFAULT '0',
  `mined` int unsigned NOT NULL DEFAULT '0',
  `herbed` int unsigned NOT NULL DEFAULT '0',
  `upgrades` int unsigned NOT NULL DEFAULT '0' COMMENT 'Gear upgrades found',
  `quality_poor` int unsigned NOT NULL DEFAULT '0' COMMENT 'Gray items looted',
  `quality_common` int unsigned NOT NULL DEFAULT '0' COMMENT 'White items looted',
  `quality_uncommon` int unsigned NOT NULL DEFAULT '0' COMMENT 'Green items looted',
  `quality_rare` int unsigned NOT NULL DEFAULT '0' COMMENT 'Blue items looted',
  `quality_epic` int unsigned NOT NULL DEFAULT '0' COMMENT 'Purple items looted',
  `quality_legendary` int unsigned NOT NULL DEFAULT '0' COMMENT 'Orange items looted',
  `filtered_poor` int unsigned NOT NULL DEFAULT '0' COMMENT 'Gray items filtered',
  `filtered_common` int unsigned NOT NULL DEFAULT '0' COMMENT 'White items filtered',
  `filtered_uncommon` int unsigned NOT NULL DEFAULT '0' COMMENT 'Green items filtered',
  `filtered_rare` int unsigned NOT NULL DEFAULT '0' COMMENT 'Blue items filtered',
  `filtered_epic` int unsigned NOT NULL DEFAULT '0' COMMENT 'Purple items filtered',
  `filtered_legendary` int unsigned NOT NULL DEFAULT '0' COMMENT 'Orange items filtered',
  `mythic_bonus_items` int unsigned NOT NULL DEFAULT '0' COMMENT 'Bonus items from M+ runs',
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`player_guid`),
  KEY `idx_quality_epic` (`quality_epic`),
  KEY `idx_quality_rare` (`quality_rare`),
  KEY `idx_quality_legendary` (`quality_legendary`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci COMMENT='DarkChaos AoE Loot - Detailed Statistics';
