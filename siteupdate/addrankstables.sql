
DROP TABLE IF EXISTS travelerMileageStats;
DROP TABLE IF EXISTS clinchedActiveStats;
DROP TABLE IF EXISTS clinchedActivePreviewStats;

CREATE TABLE travelerMileageStats (
    traveler VARCHAR(255) PRIMARY KEY,
    totalActiveMileage DECIMAL(10,2),
    rankActiveMileage INT,
    clinchedActiveMileage DECIMAL(10,2),
    activePercentage DECIMAL(5,2),
    totalActivePreviewMileage DECIMAL(10,2),
    rankActivePreviewMileage INT,
    clinchedActivePreviewMileage DECIMAL(10,2),
    activePreviewPercentage DECIMAL(5,2)
);

INSERT INTO travelerMileageStats (traveler, totalActiveMileage, rankActiveMileage, clinchedActiveMileage, activePercentage, totalActivePreviewMileage, rankActivePreviewMileage, clinchedActivePreviewMileage, activePreviewPercentage)
WITH RankedMileage AS (
  SELECT traveler,
  ROUND(SUM(o.activeMileage), 2) AS totalActiveMileage,
  RANK() OVER (ORDER BY SUM(o.activeMileage) DESC) AS rankActiveMileage,
  ROUND(SUM(COALESCE(co.activeMileage, 0)), 2) AS clinchedActiveMileage,
  ROUND(SUM(COALESCE(co.activeMileage, 0)) / SUM(o.activeMileage) * 100, 2) AS activePercentage,
  ROUND(SUM(o.activePreviewMileage), 2) AS totalActivePreviewMileage,
  RANK() OVER (ORDER BY SUM(o.activePreviewMileage) DESC) AS rankActivePreviewMileage,
  ROUND(SUM(COALESCE(co.activePreviewMileage, 0)), 2) AS clinchedActivePreviewMileage,
  ROUND(SUM(COALESCE(co.activePreviewMileage, 0)) / SUM(o.activePreviewMileage) * 100, 2) AS activePreviewPercentage
  FROM overallMileageByRegion o
  LEFT JOIN clinchedOverallMileageByRegion co ON co.region = o.region
  WHERE traveler IS NOT NULL
  GROUP BY traveler
)
SELECT * FROM RankedMileage;


CREATE TABLE clinchedActiveStats AS
WITH ActiveRoutes AS (
    SELECT COUNT(cr.route) AS total
    FROM connectedRoutes AS cr
    LEFT JOIN systems ON cr.systemName = systems.systemName
    WHERE systems.level = 'active'
),
TravelerStats AS (
    SELECT
        ccr.traveler,
        COUNT(ccr.route) AS driven,
        SUM(ccr.clinched) AS clinched,
        (SELECT total FROM ActiveRoutes) AS activeRoutes,
        ROUND(COUNT(ccr.route) / (SELECT total FROM ActiveRoutes) * 100, 2) AS drivenPercent,
        ROUND(SUM(ccr.clinched) / (SELECT total FROM ActiveRoutes) * 100, 2) AS clinchedPercent
    FROM
        connectedRoutes AS cr
    LEFT JOIN
        clinchedConnectedRoutes AS ccr ON cr.firstRoot = ccr.route
    LEFT JOIN
        routes ON ccr.route = routes.root
    LEFT JOIN
        systems ON routes.systemName = systems.systemName
    WHERE
        systems.level = 'active'
    GROUP BY
        ccr.traveler
)
SELECT
    ts.*,
    RANK() OVER (ORDER BY ts.driven DESC) AS drivenRank,
    RANK() OVER (ORDER BY ts.clinched DESC) AS clinchedRank
FROM TravelerStats ts;

CREATE TABLE clinchedActivePreviewStats AS
WITH ActivePreviewRoutes AS (
    SELECT COUNT(cr.route) AS total
    FROM connectedRoutes AS cr
    LEFT JOIN systems ON cr.systemName = systems.systemName
    WHERE systems.level = 'active' OR systems.level = 'preview'
),
TravelerStats AS (
    SELECT
        ccr.traveler,
        COUNT(ccr.route) AS driven,
        SUM(ccr.clinched) AS clinched,
        (SELECT total FROM ActivePreviewRoutes) AS activePreviewRoutes,
        ROUND(COUNT(ccr.route) / (SELECT total FROM ActivePreviewRoutes) * 100, 2) AS drivenPercent,
        ROUND(SUM(ccr.clinched) / (SELECT total FROM ActivePreviewRoutes) * 100, 2) AS clinchedPercent
    FROM
        connectedRoutes AS cr
    LEFT JOIN
        clinchedConnectedRoutes AS ccr ON cr.firstRoot = ccr.route
    LEFT JOIN
        routes ON ccr.route = routes.root
    LEFT JOIN
        systems ON routes.systemName = systems.systemName
    WHERE
        systems.level = 'active' OR systems.level = 'preview'
    GROUP BY
        ccr.traveler
)
SELECT
    ts.*,
    RANK() OVER (ORDER BY ts.driven DESC) AS drivenRank,
    RANK() OVER (ORDER BY ts.clinched DESC) AS clinchedRank
FROM TravelerStats ts;
