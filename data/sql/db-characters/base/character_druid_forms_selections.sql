CREATE TABLE IF NOT EXISTS `character_druid_forms_selections` (
    `guid` INT UNSIGNED NOT NULL COMMENT 'GUID del jugador',
    `form_type` VARCHAR(20) NOT NULL COMMENT 'Tipo de forma (cat, bear, travel, etc)',
    `display_id` INT UNSIGNED NOT NULL COMMENT 'El ID del modelo seleccionado',
    PRIMARY KEY (`guid`, `form_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Guarda las selecciones persistentes de las formas del druida';