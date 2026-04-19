package com.xwhqsj.example;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * SqlEcho -- a lightweight SQL-echo demo for JNI integration.
 *
 * Accepts a SQL-like string and a database name, parses SELECT statements,
 * and returns a structured result describing what *would* be queried.
 * No JDBC driver or real database is involved.
 *
 * Supported syntax (case-insensitive):
 *   SELECT columns FROM table [WHERE condition]
 *
 * Called from C++ via JNI:
 *   String result = sqlEcho.execute(sql, dbName);
 */
public class SqlEcho {

    private static final Pattern SELECT_PATTERN = Pattern.compile(
        "^\\s*SELECT\\s+(.+?)\\s+FROM\\s+(\\w+)(?:\\s+WHERE\\s+(.+))?\\s*$",
        Pattern.CASE_INSENSITIVE
    );

    /**
     * Parse and echo the SQL statement, returning a structured result string.
     *
     * @param sql    SQL-like input, e.g. "SELECT name, age FROM users WHERE id = 1"
     * @param dbName database name, e.g. "mydb"
     * @return structured result describing the parsed query
     */
    public String execute(String sql, String dbName) {
        if (sql == null || sql.trim().isEmpty()) {
            return errorResult("empty SQL input");
        }
        if (dbName == null || dbName.trim().isEmpty()) {
            return errorResult("empty database name");
        }

        Matcher m = SELECT_PATTERN.matcher(sql.trim());
        if (!m.matches()) {
            return errorResult("unsupported SQL: only SELECT ... FROM ... [WHERE ...] is supported");
        }

        String columnsRaw = m.group(1).trim();
        String table = m.group(2).trim();
        String whereClause = m.group(3);

        List<String> columns = parseColumns(columnsRaw);

        StringBuilder sb = new StringBuilder();
        sb.append("{\n");
        sb.append("  \"status\": \"OK\",\n");
        sb.append("  \"database\": \"").append(dbName).append("\",\n");
        sb.append("  \"table\": \"").append(table).append("\",\n");
        sb.append("  \"columns\": [");
        for (int i = 0; i < columns.size(); i++) {
            if (i > 0) sb.append(", ");
            sb.append("\"").append(columns.get(i)).append("\"");
        }
        sb.append("],\n");

        if (whereClause != null && !whereClause.trim().isEmpty()) {
            sb.append("  \"where\": \"").append(whereClause.trim()).append("\",\n");
        }

        sb.append("  \"action\": \"would query table '")
          .append(table).append("' in database '").append(dbName).append("'\",\n");
        sb.append("  \"row_count\": ").append(generateMockRowCount(table)).append(",\n");
        sb.append("  \"mock_rows\": ");
        appendMockRows(sb, columns, table);
        sb.append("\n}");

        return sb.toString();
    }

    /**
     * Static entry point for JNI -- matches the signature used by existing C++ code.
     * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
     */
    public static String process(String sql, String dbName) {
        return new SqlEcho().execute(sql, dbName);
    }

    // --- Internals ---

    private List<String> parseColumns(String raw) {
        if ("*".equals(raw.trim())) {
            return Arrays.asList("*");
        }
        List<String> result = new ArrayList<>();
        for (String col : raw.split(",")) {
            String trimmed = col.trim();
            if (!trimmed.isEmpty()) {
                result.add(trimmed);
            }
        }
        return result;
    }

    private int generateMockRowCount(String table) {
        // Deterministic mock based on table name length
        return (table.hashCode() & 0x7FFFFFFF) % 100 + 1;
    }

    private void appendMockRows(StringBuilder sb, List<String> columns, String table) {
        sb.append("[\n");
        int mockCount = Math.min(3, generateMockRowCount(table));
        List<String> resolvedCols = columns;
        if (columns.size() == 1 && "*".equals(columns.get(0))) {
            resolvedCols = Arrays.asList("id", "name", "value");
        }

        for (int row = 0; row < mockCount; row++) {
            sb.append("    {");
            for (int c = 0; c < resolvedCols.size(); c++) {
                if (c > 0) sb.append(", ");
                String col = resolvedCols.get(c);
                sb.append("\"").append(col).append("\": ");
                sb.append("\"").append(mockValue(col, row)).append("\"");
            }
            sb.append("}");
            if (row < mockCount - 1) sb.append(",");
            sb.append("\n");
        }
        sb.append("  ]");
    }

    private String mockValue(String column, int rowIndex) {
        String lower = column.toLowerCase();
        if (lower.contains("id")) {
            return String.valueOf(rowIndex + 1);
        } else if (lower.contains("name")) {
            String[] names = {"alice", "bob", "charlie"};
            return names[rowIndex % names.length];
        } else if (lower.contains("age")) {
            return String.valueOf(25 + rowIndex);
        } else if (lower.contains("email")) {
            String[] emails = {"alice@example.com", "bob@example.com", "charlie@example.com"};
            return emails[rowIndex % emails.length];
        }
        return "mock_" + rowIndex;
    }

    private String errorResult(String message) {
        return "{\n  \"status\": \"ERROR\",\n  \"message\": \"" + message + "\"\n}";
    }

    // --- main for standalone testing ---
    public static void main(String[] args) {
        SqlEcho echo = new SqlEcho();

        System.out.println("=== SqlEcho Demo ===\n");

        String[] queries = {
            "SELECT * FROM users",
            "SELECT name, age FROM employees WHERE department = 'engineering'",
            "SELECT id, email FROM customers WHERE active = true",
            "invalid sql here"
        };

        for (String sql : queries) {
            System.out.println("SQL: " + sql);
            System.out.println("DB:  testdb");
            System.out.println(echo.execute(sql, "testdb"));
            System.out.println();
        }
    }
}
