/** @type {import('jest').Config} */
module.exports = {
  preset: 'ts-jest',
  testEnvironment: 'node',
  testMatch: ['**/*.test.ts'],
  moduleFileExtensions: ['ts', 'js', 'json'],
  // Integration tests spawn real processes — allow generous timeouts.
  testTimeout: 15000,
  forceExit: true,
};
