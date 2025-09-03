// Simple JavaScript test to verify Jest configuration

describe('Basic Jest configuration test', () => {
  it('should run simple tests successfully', () => {
    expect(1 + 1).toBe(2);
  });

  it('should handle objects and arrays', () => {
    const obj = { a: 1, b: 2 };
    const arr = [1, 2, 3];
    
    expect(obj.a).toBe(1);
    expect(arr.length).toBe(3);
    expect(arr).toContain(2);
  });
});