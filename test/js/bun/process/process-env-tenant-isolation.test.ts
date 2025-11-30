import { test, expect } from "bun:test";
import { bunEnv, bunExe } from "harness";

// ════════════════════════════════════════════════════════════════
// Phase 2: Basic process.env isolation tests
//
// NOTE: Full tenant isolation tests will be added in Phase 5
// when TenantManager API is exposed to JavaScript.
//
// For now, we validate that:
// 1. process.env works normally without TenantContext
// 2. Code doesn't break for existing functionality
// ════════════════════════════════════════════════════════════════

test("process.env without TenantContext works normally - PATH exists", async () => {
  await using proc = Bun.spawn({
    cmd: [bunExe(), "-e", "console.log(process.env.PATH ? 'has-path' : 'no-path')"],
    env: bunEnv,
    stdout: "pipe",
    stderr: "pipe",
  });

  const [stdout, stderr, exitCode] = await Promise.all([
    proc.stdout.text(),
    proc.stderr.text(),
    proc.exited,
  ]);

  expect(stdout.trim()).toBe("has-path");
  expect(exitCode).toBe(0);
});

test("process.env without TenantContext can read custom env var", async () => {
  const testEnv = {
    ...bunEnv,
    TEST_CUSTOM_VAR: "test-value-123",
  };

  await using proc = Bun.spawn({
    cmd: [bunExe(), "-e", "console.log(process.env.TEST_CUSTOM_VAR)"],
    env: testEnv,
    stdout: "pipe",
    stderr: "pipe",
  });

  const [stdout, stderr, exitCode] = await Promise.all([
    proc.stdout.text(),
    proc.stderr.text(),
    proc.exited,
  ]);

  expect(stdout.trim()).toBe("test-value-123");
  expect(exitCode).toBe(0);
});

test("process.env without TenantContext can set env vars", async () => {
  await using proc = Bun.spawn({
    cmd: [
      bunExe(),
      "-e",
      `
      process.env.MY_NEW_VAR = "dynamic-value";
      console.log(process.env.MY_NEW_VAR);
      `,
    ],
    env: bunEnv,
    stdout: "pipe",
    stderr: "pipe",
  });

  const [stdout, stderr, exitCode] = await Promise.all([
    proc.stdout.text(),
    proc.stderr.text(),
    proc.exited,
  ]);

  expect(stdout.trim()).toBe("dynamic-value");
  expect(exitCode).toBe(0);
});

test("process.env without TenantContext returns undefined for non-existent var", async () => {
  await using proc = Bun.spawn({
    cmd: [
      bunExe(),
      "-e",
      "console.log(typeof process.env.THIS_VAR_DOES_NOT_EXIST_123456)",
    ],
    env: bunEnv,
    stdout: "pipe",
    stderr: "pipe",
  });

  const [stdout, stderr, exitCode] = await Promise.all([
    proc.stdout.text(),
    proc.stderr.text(),
    proc.exited,
  ]);

  expect(stdout.trim()).toBe("undefined");
  expect(exitCode).toBe(0);
});

// ════════════════════════════════════════════════════════════════
// TODO: Add tenant isolation tests in Phase 5
//
// Example test structure for Phase 5:
//
// test("process.env with TenantContext isolates environment", async () => {
//   const { TenantManager } = await import("bun:tenant");
//
//   const tenant1 = TenantManager.create({
//     env: { SECRET: "tenant-1-secret" }
//   });
//
//   const tenant2 = TenantManager.create({
//     env: { SECRET: "tenant-2-secret" }
//   });
//
//   await TenantManager.run(tenant1, () => {
//     expect(process.env.SECRET).toBe("tenant-1-secret");
//     expect(process.env.PATH).toBeUndefined(); // Isolation!
//   });
//
//   await TenantManager.run(tenant2, () => {
//     expect(process.env.SECRET).toBe("tenant-2-secret");
//   });
// });
// ════════════════════════════════════════════════════════════════
