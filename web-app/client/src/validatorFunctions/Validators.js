//  functions for fields
export const fileExistence = (file) => !!file;
export const fileSize = (file, maxfilesize) => !!file && file.size <= maxfilesize;
// eslint-disable-next-line max-len
export const fileFormat = (file, allowedFileFormats) => !!file && allowedFileFormats.indexOf(file.type) !== -1;
// eslint-disable-next-line max-len
export const separator = (n, allowedSeparators) => !!n && allowedSeparators.indexOf(n) !== -1;
export const error = (n) => n === 0 || (!!n && n >= 0 && n <= 1);
export const maxLHS = (n) => !Number.isNaN(n) && n > 0 && n % 1 === 0;
